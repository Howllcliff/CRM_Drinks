#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#error This C version currently targets Windows with Winsock.
#endif

#define PORT 3000
#define MAX_USERS 256
#define MAX_DRINKS 1024
#define MAX_REQUEST_SIZE 65535
#define MAX_LINE 2048
#define MAX_SMALL 256
#define MAX_MEDIUM 512
#define MAX_LARGE 4096
#define TOKEN_TTL_SECONDS (7LL * 24LL * 60LL * 60LL)
#define DB_PATH "backend/data/db.json"
#define FRONTEND_ROOT "../frontend"
#define JWT_SECRET_FALLBACK "dev_local_secret_change_me"

typedef struct {
    char id[37];
    char email[128];
    char password_hash[160];
    char created_at[40];
} User;

typedef struct {
    char id[37];
    char user_id[37];
    char name[128];
    double cost;
    double price;
    double cmv;
    char spirits[2048];
    char ingredients[2048];
    char date[40];
} Drink;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    Drink drinks[MAX_DRINKS];
    int drink_count;
} Database;

typedef struct {
    char user_id[37];
    char email[128];
    long long exp;
} Claims;

typedef struct {
    char method[8];
    char path[256];
    char authorization[512];
    const char *body;
} HttpRequest;

static const char *jwt_secret(void) {
    const char *value = getenv("JWT_SECRET");
    return (value && value[0]) ? value : JWT_SECRET_FALLBACK;
}

static int port_value(void) {
    const char *value = getenv("PORT");
    if (!value || !value[0]) {
        return PORT;
    }
    return atoi(value);
}

static unsigned long long fnv1a64(const char *text) {
    unsigned long long hash = 1469598103934665603ULL;
    while (text && *text) {
        hash ^= (unsigned char)*text++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void random_hex(char *output, size_t length) {
    static const char digits[] = "0123456789abcdef";
    for (size_t index = 0; index < length; ++index) {
        output[index] = digits[rand() % 16];
    }
    output[length] = '\0';
}

static void generate_uuid(char *output, size_t size) {
    char hex[33];
    random_hex(hex, 32);
    snprintf(output, size, "%.8s-%.4s-%.4s-%.4s-%.12s", hex, hex + 8, hex + 12, hex + 16, hex + 20);
}

static void format_hash(const char *text, char *output, size_t size) {
    snprintf(output, size, "%016llx", (unsigned long long)fnv1a64(text));
}

static void hash_password(const char *password, char *output, size_t size) {
    char salt[17];
    char payload[MAX_MEDIUM];
    char digest[17];

    random_hex(salt, 16);
    snprintf(payload, sizeof(payload), "%s|%s|%s", password, salt, jwt_secret());
    format_hash(payload, digest, sizeof(digest));
    snprintf(output, size, "%s$%s", salt, digest);
}

static int verify_password(const char *stored_hash, const char *password) {
    char salt[17];
    char digest[17];
    char expected[160];
    char payload[MAX_MEDIUM];
    const char *separator = strchr(stored_hash, '$');

    if (!separator) {
        return 0;
    }

    size_t salt_length = (size_t)(separator - stored_hash);
    if (salt_length == 0 || salt_length >= sizeof(salt)) {
        return 0;
    }

    memcpy(salt, stored_hash, salt_length);
    salt[salt_length] = '\0';
    snprintf(payload, sizeof(payload), "%s|%s|%s", password, salt, jwt_secret());
    format_hash(payload, digest, sizeof(digest));
    snprintf(expected, sizeof(expected), "%s$%s", salt, digest);
    return strcmp(stored_hash, expected) == 0;
}

static void json_escape(const char *input, char *output, size_t size) {
    size_t out_index = 0;
    for (size_t i = 0; input && input[i] != '\0'; ++i) {
        const char *replacement = NULL;
        char buffer[2] = {0, 0};
        switch (input[i]) {
            case '\\': replacement = "\\\\"; break;
            case '"': replacement = "\\\""; break;
            case '\n': replacement = "\\n"; break;
            case '\r': replacement = "\\r"; break;
            case '\t': replacement = "\\t"; break;
            default:
                buffer[0] = input[i];
                replacement = buffer;
                break;
        }

        size_t needed = strlen(replacement);
        if (out_index + needed + 1 >= size) {
            break;
        }
        memcpy(output + out_index, replacement, needed);
        out_index += needed;
    }
    output[out_index] = '\0';
}

static char *read_entire_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long length;
    char *buffer;

    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    length = ftell(file);
    if (length < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char *)malloc((size_t)length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)length, file) != (size_t)length) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

static void ensure_db_file(void) {
    FILE *file = fopen(DB_PATH, "rb");
    if (file) {
        fclose(file);
        return;
    }

    file = fopen(DB_PATH, "wb");
    if (!file) {
        return;
    }

    fputs("{\n  \"users\": [],\n  \"drinks\": []\n}\n", file);
    fclose(file);
}

static const char *skip_spaces(const char *text) {
    while (text && *text && isspace((unsigned char)*text)) {
        ++text;
    }
    return text;
}

static const char *find_json_key(const char *text, const char *key) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(text, pattern);
}

static const char *match_delimited(const char *start, char open_char, char close_char) {
    int depth = 0;
    int in_string = 0;
    const char *cursor = start;

    if (!cursor || *cursor != open_char) {
        return NULL;
    }

    while (*cursor) {
        char current = *cursor;
        if (in_string) {
            if (current == '\\' && cursor[1] != '\0') {
                cursor += 2;
                continue;
            }
            if (current == '"') {
                in_string = 0;
            }
            ++cursor;
            continue;
        }

        if (current == '"') {
            in_string = 1;
        } else if (current == open_char) {
            ++depth;
        } else if (current == close_char) {
            --depth;
            if (depth == 0) {
                return cursor;
            }
        }

        ++cursor;
    }

    return NULL;
}

static int extract_json_string(const char *object, const char *key, char *output, size_t size) {
    const char *cursor = find_json_key(object, key);
    if (!cursor) {
        return 0;
    }

    cursor = strchr(cursor, ':');
    if (!cursor) {
        return 0;
    }

    cursor = skip_spaces(cursor + 1);
    if (*cursor != '"') {
        return 0;
    }

    ++cursor;
    size_t out_index = 0;
    while (*cursor && *cursor != '"') {
        char value = *cursor;
        if (value == '\\' && cursor[1] != '\0') {
            ++cursor;
            switch (*cursor) {
                case 'n': value = '\n'; break;
                case 'r': value = '\r'; break;
                case 't': value = '\t'; break;
                case '"': value = '"'; break;
                case '\\': value = '\\'; break;
                default: value = *cursor; break;
            }
        }

        if (out_index + 1 < size) {
            output[out_index++] = value;
        }
        ++cursor;
    }

    output[out_index] = '\0';
    return 1;
}

static int extract_json_number(const char *object, const char *key, double *output) {
    const char *cursor = find_json_key(object, key);
    char *end = NULL;

    if (!cursor) {
        return 0;
    }

    cursor = strchr(cursor, ':');
    if (!cursor) {
        return 0;
    }

    cursor = skip_spaces(cursor + 1);
    errno = 0;
    *output = strtod(cursor, &end);
    return (end != cursor && errno == 0) ? 1 : 0;
}

static int extract_json_array_raw(const char *object, const char *key, char *output, size_t size) {
    const char *cursor = find_json_key(object, key);
    const char *end = NULL;

    if (!cursor) {
        return 0;
    }

    cursor = strchr(cursor, ':');
    if (!cursor) {
        return 0;
    }

    cursor = skip_spaces(cursor + 1);
    if (*cursor != '[') {
        return 0;
    }

    end = match_delimited(cursor, '[', ']');
    if (!end) {
        return 0;
    }

    size_t length = (size_t)(end - cursor + 1);
    if (length + 1 > size) {
        length = size - 1;
    }

    memcpy(output, cursor, length);
    output[length] = '\0';
    return 1;
}

static void database_init(Database *database) {
    database->user_count = 0;
    database->drink_count = 0;
}

static void parse_user_object(const char *object, Database *database) {
    if (database->user_count >= MAX_USERS) {
        return;
    }

    User *user = &database->users[database->user_count];
    memset(user, 0, sizeof(*user));
    extract_json_string(object, "id", user->id, sizeof(user->id));
    extract_json_string(object, "email", user->email, sizeof(user->email));
    extract_json_string(object, "passwordHash", user->password_hash, sizeof(user->password_hash));
    extract_json_string(object, "createdAt", user->created_at, sizeof(user->created_at));
    if (user->id[0] && user->email[0] && user->password_hash[0]) {
        database->user_count++;
    }
}

static void parse_drink_object(const char *object, Database *database) {
    if (database->drink_count >= MAX_DRINKS) {
        return;
    }

    Drink *drink = &database->drinks[database->drink_count];
    memset(drink, 0, sizeof(*drink));
    extract_json_string(object, "id", drink->id, sizeof(drink->id));
    extract_json_string(object, "userId", drink->user_id, sizeof(drink->user_id));
    extract_json_string(object, "name", drink->name, sizeof(drink->name));
    extract_json_number(object, "cost", &drink->cost);
    extract_json_number(object, "price", &drink->price);
    extract_json_number(object, "cmv", &drink->cmv);
    extract_json_array_raw(object, "spirits", drink->spirits, sizeof(drink->spirits));
    extract_json_array_raw(object, "ingredients", drink->ingredients, sizeof(drink->ingredients));
    extract_json_string(object, "date", drink->date, sizeof(drink->date));
    if (drink->id[0] && drink->user_id[0] && drink->name[0]) {
        database->drink_count++;
    }
}

static void parse_array_items(const char *array_text, void (*item_parser)(const char *, Database *), Database *database) {
    const char *cursor = array_text;
    if (!cursor) {
        return;
    }

    while (*cursor && *cursor != '[') {
        ++cursor;
    }

    if (*cursor != '[') {
        return;
    }

    ++cursor;
    while (*cursor && *cursor != ']') {
        cursor = skip_spaces(cursor);
        if (*cursor == '{') {
            const char *end = match_delimited(cursor, '{', '}');
            if (!end) {
                break;
            }

            size_t length = (size_t)(end - cursor + 1);
            char *object = (char *)malloc(length + 1);
            if (!object) {
                return;
            }

            memcpy(object, cursor, length);
            object[length] = '\0';
            item_parser(object, database);
            free(object);
            cursor = end + 1;
            continue;
        }

        ++cursor;
    }
}

static void load_database(Database *database) {
    char *content;
    const char *users_array;
    const char *drinks_array;

    database_init(database);
    ensure_db_file();

    content = read_entire_file(DB_PATH);
    if (!content) {
        return;
    }

    users_array = find_json_key(content, "users");
    if (users_array) {
        users_array = strchr(users_array, '[');
        parse_array_items(users_array, parse_user_object, database);
    }

    drinks_array = find_json_key(content, "drinks");
    if (drinks_array) {
        drinks_array = strchr(drinks_array, '[');
        parse_array_items(drinks_array, parse_drink_object, database);
    }

    free(content);
}

static void write_database(const Database *database) {
    FILE *file = fopen(DB_PATH, "wb");
    if (!file) {
        return;
    }

    fprintf(file, "{\n  \"users\": [\n");
    for (int index = 0; index < database->user_count; ++index) {
        char id[128], email[160], hash[192], created_at[64];
        const User *user = &database->users[index];
        json_escape(user->id, id, sizeof(id));
        json_escape(user->email, email, sizeof(email));
        json_escape(user->password_hash, hash, sizeof(hash));
        json_escape(user->created_at, created_at, sizeof(created_at));
        fprintf(file,
                "    {\n"
                "      \"id\": \"%s\",\n"
                "      \"email\": \"%s\",\n"
                "      \"passwordHash\": \"%s\",\n"
                "      \"createdAt\": \"%s\"\n"
                "    }%s\n",
                id,
                email,
                hash,
                created_at,
                (index + 1 < database->user_count) ? "," : "");
    }

    fprintf(file, "  ],\n  \"drinks\": [\n");
    for (int index = 0; index < database->drink_count; ++index) {
        char id[128], user_id[128], name[160], spirits[4096], ingredients[4096], date[64];
        const Drink *drink = &database->drinks[index];
        json_escape(drink->id, id, sizeof(id));
        json_escape(drink->user_id, user_id, sizeof(user_id));
        json_escape(drink->name, name, sizeof(name));
        json_escape(drink->spirits[0] ? drink->spirits : "[]", spirits, sizeof(spirits));
        json_escape(drink->ingredients[0] ? drink->ingredients : "[]", ingredients, sizeof(ingredients));
        json_escape(drink->date, date, sizeof(date));
        fprintf(file,
                "    {\n"
                "      \"id\": \"%s\",\n"
                "      \"userId\": \"%s\",\n"
                "      \"name\": \"%s\",\n"
                "      \"cost\": %.2f,\n"
                "      \"price\": %.2f,\n"
                "      \"cmv\": %.2f,\n"
                "      \"spirits\": %s,\n"
                "      \"ingredients\": %s,\n"
                "      \"date\": \"%s\"\n"
                "    }%s\n",
                id,
                user_id,
                name,
                drink->cost,
                drink->price,
                drink->cmv,
                spirits,
                ingredients,
                date,
                (index + 1 < database->drink_count) ? "," : "");
    }

    fprintf(file, "  ]\n}\n");
    fclose(file);
}

static User *find_user_by_email(Database *database, const char *email) {
    for (int index = 0; index < database->user_count; ++index) {
        if (_stricmp(database->users[index].email, email) == 0) {
            return &database->users[index];
        }
    }
    return NULL;
}

static int find_drink_index(Database *database, const char *id, const char *user_id) {
    for (int index = 0; index < database->drink_count; ++index) {
        if (strcmp(database->drinks[index].id, id) == 0 && strcmp(database->drinks[index].user_id, user_id) == 0) {
            return index;
        }
    }
    return -1;
}

static void create_token(const char *user_id, const char *email, char *output, size_t size) {
    long long exp = (long long)time(NULL) + TOKEN_TTL_SECONDS;
    char payload[MAX_MEDIUM];
    char signature_input[MAX_LARGE];

    snprintf(payload, sizeof(payload), "%s|%s|%lld", user_id, email, exp);
    snprintf(signature_input, sizeof(signature_input), "%s|%s", payload, jwt_secret());
    snprintf(output, size, "%s|%016llx", payload, fnv1a64(signature_input));
}

static int parse_token(const char *token, Claims *claims) {
    char signature[32];
    char payload[MAX_MEDIUM];
    char expected[MAX_SMALL];
    long long exp = 0;

    if (!token || !claims) {
        return 0;
    }

    if (sscanf(token, "%36[^|]|%127[^|]|%lld|%31s", claims->user_id, claims->email, &exp, signature) != 4) {
        return 0;
    }

    claims->exp = exp;
    if ((long long)time(NULL) > claims->exp) {
        return 0;
    }

    snprintf(payload, sizeof(payload), "%s|%s|%lld", claims->user_id, claims->email, claims->exp);
    snprintf(expected, sizeof(expected), "%016llx", fnv1a64((char[]){0}));
    {
        char signature_input[MAX_LARGE];
        snprintf(signature_input, sizeof(signature_input), "%s|%s", payload, jwt_secret());
        snprintf(expected, sizeof(expected), "%016llx", fnv1a64(signature_input));
    }

    return strcmp(signature, expected) == 0;
}

static const char *status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 404: return "Not Found";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

static const char *guess_content_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) {
        return "text/plain; charset=utf-8";
    }

    if (_stricmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (_stricmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (_stricmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (_stricmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (_stricmp(dot, ".svg") == 0) return "image/svg+xml";
    if (_stricmp(dot, ".png") == 0) return "image/png";
    if (_stricmp(dot, ".jpg") == 0 || _stricmp(dot, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

static void send_all(SOCKET client, const char *data, size_t length) {
    size_t sent_total = 0;
    while (sent_total < length) {
        int sent = send(client, data + sent_total, (int)(length - sent_total), 0);
        if (sent <= 0) {
            break;
        }
        sent_total += (size_t)sent;
    }
}

static void send_response(SOCKET client, int code, const char *content_type, const char *headers, const char *body) {
    char header_buffer[2048];
    size_t body_length = body ? strlen(body) : 0;
    int written = snprintf(header_buffer, sizeof(header_buffer),
                           "HTTP/1.1 %d %s\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                           "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                           "Connection: close\r\n"
                           "%s"
                           "%s"
                           "Content-Length: %zu\r\n\r\n",
                           code,
                           status_text(code),
                           content_type ? "Content-Type: " : "",
                           content_type ? content_type : "",
                           body_length);

    if (written > 0) {
        send_all(client, header_buffer, (size_t)written);
    }

    if (headers && headers[0]) {
        send_all(client, headers, strlen(headers));
    }

    if (body && body_length > 0) {
        send_all(client, body, body_length);
    }
}

static void send_json(SOCKET client, int code, const char *body) {
    send_response(client, code, "application/json; charset=utf-8", NULL, body);
}

static void send_text(SOCKET client, int code, const char *content_type, const char *body) {
    send_response(client, code, content_type, NULL, body);
}

static void send_redirect(SOCKET client, const char *location) {
    char headers[512];
    snprintf(headers, sizeof(headers), "Location: %s\r\n", location);
    send_response(client, 302, "text/plain; charset=utf-8", headers, "Redirecting...");
}

static const char *find_header_value(const char *request, const char *header_name, char *output, size_t size) {
    const char *cursor = request;
    size_t header_length = strlen(header_name);

    while (cursor && *cursor) {
        const char *line_end = strstr(cursor, "\r\n");
        if (!line_end) {
            break;
        }

        if ((size_t)(line_end - cursor) > header_length && _strnicmp(cursor, header_name, header_length) == 0 && cursor[header_length] == ':') {
            const char *value = skip_spaces(cursor + header_length + 1);
            size_t value_length = (size_t)(line_end - value);
            if (value_length >= size) {
                value_length = size - 1;
            }
            memcpy(output, value, value_length);
            output[value_length] = '\0';
            return output;
        }

        cursor = line_end + 2;
        if (cursor[0] == '\r' && cursor[1] == '\n') {
            break;
        }
    }

    if (size > 0) {
        output[0] = '\0';
    }
    return NULL;
}

static const char *request_body(const char *request) {
    const char *body = strstr(request, "\r\n\r\n");
    return body ? body + 4 : "";
}

static int request_is_json(const HttpRequest *request) {
    (void)request;
    return 1;
}

static int parse_request(const char *raw, HttpRequest *request) {
    const char *body_start;
    memset(request, 0, sizeof(*request));
    if (sscanf(raw, "%7s %255s", request->method, request->path) != 2) {
        return 0;
    }

    find_header_value(raw, "Authorization", request->authorization, sizeof(request->authorization));
    body_start = request_body(raw);
    request->body = body_start;
    return 1;
}

static int body_has_prefix(const char *body, const char *prefix) {
    while (*prefix) {
        if (*body++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static int extract_request_string(const char *body, const char *key, char *output, size_t size) {
    return extract_json_string(body, key, output, size);
}

static int extract_request_number(const char *body, const char *key, double *output) {
    return extract_json_number(body, key, output);
}

static int extract_request_array(const char *body, const char *key, char *output, size_t size) {
    return extract_json_array_raw(body, key, output, size);
}

static void sort_drinks_by_date(Database *database) {
    for (int i = 0; i < database->drink_count; ++i) {
        for (int j = i + 1; j < database->drink_count; ++j) {
            if (strcmp(database->drinks[j].date, database->drinks[i].date) > 0) {
                Drink temp = database->drinks[i];
                database->drinks[i] = database->drinks[j];
                database->drinks[j] = temp;
            }
        }
    }
}

static void handle_health(SOCKET client) {
    send_json(client, 200, "{\"ok\":true,\"message\":\"Servidor online.\"}");
}

static void handle_register(SOCKET client, const char *body) {
    Database database;
    char email[128];
    char password[128];
    char hashed[160];
    char user_id[37];
    char created_at[40];
    char token[512];

    load_database(&database);
    email[0] = '\0';
    password[0] = '\0';

    if (!extract_request_string(body, "email", email, sizeof(email)) || !extract_request_string(body, "password", password, sizeof(password))) {
        send_json(client, 400, "{\"message\":\"E-mail e senha sao obrigatorios.\"}");
        return;
    }

    if (strlen(password) < 6) {
        send_json(client, 400, "{\"message\":\"A senha deve ter pelo menos 6 caracteres.\"}");
        return;
    }

    for (char *p = email; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }

    if (find_user_by_email(&database, email)) {
        send_json(client, 409, "{\"message\":\"Este e-mail ja esta cadastrado.\"}");
        return;
    }

    if (database.user_count >= MAX_USERS) {
        send_json(client, 500, "{\"message\":\"Limite de usuarios atingido.\"}");
        return;
    }

    generate_uuid(user_id, sizeof(user_id));
    hash_password(password, hashed, sizeof(hashed));
    snprintf(created_at, sizeof(created_at), "%lld", (long long)time(NULL));

    User *user = &database.users[database.user_count++];
    memset(user, 0, sizeof(*user));
    strcpy(user->id, user_id);
    strcpy(user->email, email);
    strcpy(user->password_hash, hashed);
    strcpy(user->created_at, created_at);

    write_database(&database);
    create_token(user->id, user->email, token, sizeof(token));

    {
        char response[768];
        snprintf(response, sizeof(response), "{\"token\":\"%s\"}", token);
        send_json(client, 201, response);
    }
}

static void handle_login(SOCKET client, const char *body) {
    Database database;
    char email[128];
    char password[128];
    char token[512];
    User *user = NULL;

    load_database(&database);
    email[0] = '\0';
    password[0] = '\0';

    if (!extract_request_string(body, "email", email, sizeof(email)) || !extract_request_string(body, "password", password, sizeof(password))) {
        send_json(client, 400, "{\"message\":\"E-mail e senha sao obrigatorios.\"}");
        return;
    }

    for (char *p = email; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }

    user = find_user_by_email(&database, email);
    if (!user || !verify_password(user->password_hash, password)) {
        send_json(client, 401, "{\"message\":\"Credenciais invalidas.\"}");
        return;
    }

    create_token(user->id, user->email, token, sizeof(token));
    {
        char response[768];
        snprintf(response, sizeof(response), "{\"token\":\"%s\"}", token);
        send_json(client, 200, response);
    }
}

static int auth_from_request(const HttpRequest *request, Claims *claims) {
    const char *token = request->authorization;
    while (*token == ' ') {
        ++token;
    }
    if (strncmp(token, "Bearer ", 7) == 0) {
        token += 7;
    }
    return parse_token(token, claims);
}

static void handle_list_drinks(SOCKET client, const HttpRequest *request) {
    Database database;
    Claims claims;
    char response[MAX_REQUEST_SIZE];
    size_t offset = 0;

    if (!auth_from_request(request, &claims)) {
        send_json(client, 401, "{\"message\":\"Token invalido ou expirado.\"}");
        return;
    }

    load_database(&database);
    sort_drinks_by_date(&database);

    offset += (size_t)snprintf(response + offset, sizeof(response) - offset, "[");
    for (int index = 0; index < database.drink_count; ++index) {
        const Drink *drink = &database.drinks[index];
        if (strcmp(drink->user_id, claims.user_id) != 0) {
            continue;
        }

        char id[128], user_id[128], name[160], spirits[4096], ingredients[4096], date[64];
        json_escape(drink->id, id, sizeof(id));
        json_escape(drink->user_id, user_id, sizeof(user_id));
        json_escape(drink->name, name, sizeof(name));
        json_escape(drink->spirits[0] ? drink->spirits : "[]", spirits, sizeof(spirits));
        json_escape(drink->ingredients[0] ? drink->ingredients : "[]", ingredients, sizeof(ingredients));
        json_escape(drink->date, date, sizeof(date));

        offset += (size_t)snprintf(response + offset, sizeof(response) - offset,
                                   "%s{\"id\":\"%s\",\"userId\":\"%s\",\"name\":\"%s\",\"cost\":%.2f,\"price\":%.2f,\"cmv\":%.2f,\"spirits\":%s,\"ingredients\":%s,\"date\":\"%s\"}",
                                   (offset > 1) ? "," : "",
                                   id, user_id, name,
                                   drink->cost,
                                   drink->price,
                                   drink->cmv,
                                   spirits,
                                   ingredients,
                                   date);
        if (offset >= sizeof(response)) {
            break;
        }
    }

    if (offset + 2 < sizeof(response)) {
        response[offset++] = ']';
        response[offset] = '\0';
    } else {
        response[sizeof(response) - 2] = ']';
        response[sizeof(response) - 1] = '\0';
    }

    send_json(client, 200, response);
}

static void handle_create_drink(SOCKET client, const HttpRequest *request) {
    Database database;
    Claims claims;
    char name[128];
    char spirits[4096];
    char ingredients[4096];
    char drink_id[37];
    char date[40];
    double cost = 0.0;
    double price = 0.0;
    double cmv = 0.0;
    char response[8192];

    if (!auth_from_request(request, &claims)) {
        send_json(client, 401, "{\"message\":\"Token invalido ou expirado.\"}");
        return;
    }

    load_database(&database);
    name[0] = '\0';
    spirits[0] = '\0';
    ingredients[0] = '\0';

    if (!extract_request_string(request->body, "name", name, sizeof(name))) {
        send_json(client, 400, "{\"message\":\"Nome e custo valido sao obrigatorios.\"}");
        return;
    }

    extract_request_number(request->body, "cost", &cost);
    extract_request_number(request->body, "price", &price);
    extract_request_number(request->body, "cmv", &cmv);
    extract_request_array(request->body, "spirits", spirits, sizeof(spirits));
    extract_request_array(request->body, "ingredients", ingredients, sizeof(ingredients));

    if (cost <= 0.0) {
        send_json(client, 400, "{\"message\":\"Nome e custo valido sao obrigatorios.\"}");
        return;
    }

    if (database.drink_count >= MAX_DRINKS) {
        send_json(client, 500, "{\"message\":\"Limite de drinks atingido.\"}");
        return;
    }

    generate_uuid(drink_id, sizeof(drink_id));
    snprintf(date, sizeof(date), "%lld", (long long)time(NULL));

    Drink *drink = &database.drinks[database.drink_count++];
    memset(drink, 0, sizeof(*drink));
    strcpy(drink->id, drink_id);
    strcpy(drink->user_id, claims.user_id);
    strncpy(drink->name, name, sizeof(drink->name) - 1);
    drink->cost = cost;
    drink->price = price;
    drink->cmv = cmv;
    strncpy(drink->spirits, spirits[0] ? spirits : "[]", sizeof(drink->spirits) - 1);
    strncpy(drink->ingredients, ingredients[0] ? ingredients : "[]", sizeof(drink->ingredients) - 1);
    strncpy(drink->date, date, sizeof(drink->date) - 1);

    write_database(&database);

    snprintf(response, sizeof(response),
             "{\"id\":\"%s\",\"userId\":\"%s\",\"name\":\"%s\",\"cost\":%.2f,\"price\":%.2f,\"cmv\":%.2f,\"spirits\":%s,\"ingredients\":%s,\"date\":\"%s\"}",
             drink->id,
             drink->user_id,
             drink->name,
             drink->cost,
             drink->price,
             drink->cmv,
             drink->spirits,
             drink->ingredients,
             drink->date);
    send_json(client, 201, response);
}

static void handle_update_drink(SOCKET client, const HttpRequest *request, const char *drink_id) {
    Database database;
    Claims claims;
    char name[128];
    char spirits[4096];
    char ingredients[4096];
    char response[8192];
    double cost = 0.0;
    double price = 0.0;
    double cmv = 0.0;
    int index;

    if (!auth_from_request(request, &claims)) {
        send_json(client, 401, "{\"message\":\"Token invalido ou expirado.\"}");
        return;
    }

    load_database(&database);
    index = find_drink_index(&database, drink_id, claims.user_id);
    if (index < 0) {
        send_json(client, 404, "{\"message\":\"Drink nao encontrado.\"}");
        return;
    }

    name[0] = '\0';
    spirits[0] = '\0';
    ingredients[0] = '\0';
    extract_request_string(request->body, "name", name, sizeof(name));
    extract_request_number(request->body, "cost", &cost);
    extract_request_number(request->body, "price", &price);
    extract_request_number(request->body, "cmv", &cmv);
    extract_request_array(request->body, "spirits", spirits, sizeof(spirits));
    extract_request_array(request->body, "ingredients", ingredients, sizeof(ingredients));

    Drink *drink = &database.drinks[index];
    if (name[0]) {
        strncpy(drink->name, name, sizeof(drink->name) - 1);
    }
    drink->cost = cost;
    drink->price = price;
    drink->cmv = cmv;
    if (spirits[0]) {
        strncpy(drink->spirits, spirits, sizeof(drink->spirits) - 1);
    }
    if (ingredients[0]) {
        strncpy(drink->ingredients, ingredients, sizeof(drink->ingredients) - 1);
    }

    write_database(&database);
    snprintf(response, sizeof(response),
             "{\"id\":\"%s\",\"userId\":\"%s\",\"name\":\"%s\",\"cost\":%.2f,\"price\":%.2f,\"cmv\":%.2f,\"spirits\":%s,\"ingredients\":%s,\"date\":\"%s\"}",
             drink->id,
             drink->user_id,
             drink->name,
             drink->cost,
             drink->price,
             drink->cmv,
             drink->spirits,
             drink->ingredients,
             drink->date);
    send_json(client, 200, response);
}

static void handle_delete_drink(SOCKET client, const HttpRequest *request, const char *drink_id) {
    Database database;
    Claims claims;
    int index;

    if (!auth_from_request(request, &claims)) {
        send_json(client, 401, "{\"message\":\"Token invalido ou expirado.\"}");
        return;
    }

    load_database(&database);
    index = find_drink_index(&database, drink_id, claims.user_id);
    if (index < 0) {
        send_json(client, 404, "{\"message\":\"Drink nao encontrado.\"}");
        return;
    }

    for (int cursor = index; cursor < database.drink_count - 1; ++cursor) {
        database.drinks[cursor] = database.drinks[cursor + 1];
    }
    database.drink_count--;
    write_database(&database);
    send_json(client, 200, "{\"message\":\"Drink removido com sucesso.\"}");
}

static void serve_static_file(SOCKET client, const char *request_path) {
    char filesystem_path[MAX_LINE];
    char *content;
    const char *content_type;

    if (strcmp(request_path, "/") == 0) {
        send_redirect(client, "/frontend/pages/auth.html");
        return;
    }

    if (strncmp(request_path, "/frontend/", 10) != 0) {
        send_text(client, 404, "text/plain; charset=utf-8", "Pagina nao encontrada.");
        return;
    }

    snprintf(filesystem_path, sizeof(filesystem_path), "%s%s", "..", request_path);
    content = read_entire_file(filesystem_path);
    if (!content) {
        send_text(client, 404, "text/plain; charset=utf-8", "Arquivo nao encontrado.");
        return;
    }

    content_type = guess_content_type(filesystem_path);
    send_text(client, 200, content_type, content);
    free(content);
}

static void handle_request(SOCKET client, const char *raw_request) {
    HttpRequest request;
    char *path_copy;
    char *drink_id;

    if (!parse_request(raw_request, &request)) {
        send_text(client, 400, "text/plain; charset=utf-8", "Requisicao invalida.");
        return;
    }

    path_copy = _strdup(request.path);
    if (!path_copy) {
        send_text(client, 500, "text/plain; charset=utf-8", "Falha de memoria.");
        return;
    }

    drink_id = strstr(path_copy, "/api/drinks/");

    if (_stricmp(request.method, "OPTIONS") == 0) {
        send_response(client, 204, "text/plain; charset=utf-8", NULL, "");
    } else if (_stricmp(request.method, "GET") == 0 && strcmp(request.path, "/api/health") == 0) {
        handle_health(client);
    } else if (_stricmp(request.method, "POST") == 0 && strcmp(request.path, "/api/auth/register") == 0) {
        handle_register(client, request.body);
    } else if (_stricmp(request.method, "POST") == 0 && strcmp(request.path, "/api/auth/login") == 0) {
        handle_login(client, request.body);
    } else if (_stricmp(request.method, "GET") == 0 && strcmp(request.path, "/api/drinks") == 0) {
        handle_list_drinks(client, &request);
    } else if (_stricmp(request.method, "POST") == 0 && strcmp(request.path, "/api/drinks") == 0) {
        handle_create_drink(client, &request);
    } else if (drink_id && _stricmp(request.method, "PUT") == 0) {
        handle_update_drink(client, &request, drink_id + strlen("/api/drinks/"));
    } else if (drink_id && _stricmp(request.method, "DELETE") == 0) {
        handle_delete_drink(client, &request, drink_id + strlen("/api/drinks/"));
    } else {
        serve_static_file(client, request.path);
    }

    free(path_copy);
}

static int receive_request(SOCKET client, char *buffer, size_t size) {
    int received = recv(client, buffer, (int)size - 1, 0);
    if (received <= 0) {
        return 0;
    }
    buffer[received] = '\0';
    return received;
}

int main(void) {
    WSADATA data;
    SOCKET server_socket;
    struct sockaddr_in address;
    char request_buffer[MAX_REQUEST_SIZE];
    int server_port = port_value();

    srand((unsigned int)time(NULL));
    ensure_db_file();

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        fprintf(stderr, "Falha ao iniciar Winsock.\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Falha ao criar socket.\n");
        WSACleanup();
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((u_short)server_port);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "Falha ao vincular porta %d.\n", server_port);
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "Falha ao ouvir conexoes.\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Servidor rodando em http://localhost:%d\n", server_port);

    for (;;) {
        struct sockaddr_in client_address;
        int client_length = sizeof(client_address);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_length);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        if (receive_request(client_socket, request_buffer, sizeof(request_buffer))) {
            handle_request(client_socket, request_buffer);
        } else {
            send_text(client_socket, 400, "text/plain; charset=utf-8", "Requisicao vazia.");
        }

        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
