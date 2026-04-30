# Sistema de Drinks em C

Versao em C do projeto, com backend proprior que expõe a mesma API usada pelo frontend existente do repositório.

## Estrutura

```text
C/
  README.md
  .env.example
  Makefile
  backend/
    data/
      db.json
    src/
      server.c
```

O backend em C serve o frontend que continua na pasta raiz do projeto:

- `../frontend/pages/auth.html`
- `../frontend/pages/drinks.html`
- `../frontend/assets/css/*`
- `../frontend/assets/js/*`

## Requisitos

- Windows
- GCC com suporte a Winsock, ou compilador compatível com `winsock2`
- `make` opcional, apenas para facilitar a compilacao

## Compilacao

Na pasta `C`:

```bash
make
```

Se preferir compilar manualmente:

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic backend/src/server.c -o drinks_c.exe -lws2_32
```

## Execucao

```bash
drinks_c.exe
```

Depois abra:

- `http://localhost:3000`

## API

- `GET /api/health`
- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/drinks`
- `POST /api/drinks`
- `PUT /api/drinks/<id>`
- `DELETE /api/drinks/<id>`

## Observacoes

- A persistencia continua em JSON local.
- O token de autenticacao e gerado e validado no proprio backend em C.
- Esta versao foi feita para estudo e portabilidade simples no Windows.
