# Documentação Completa do Projeto - Sistema de Drinks

## 1. Visão Geral

O **Sistema de Drinks** é uma aplicação web full stack para:
- cadastro e autenticação de usuários;
- criação, edição, listagem e exclusão de receitas de drinks;
- cálculo de custo total e preço sugerido com base em CMV (%).

A aplicação usa:
- **Backend:** Node.js + Express
- **Frontend:** HTML, CSS e JavaScript puro
- **Persistência:** arquivo JSON local (`backend/data/db.json`)
- **Autenticação:** JWT (JSON Web Token)

---

## 2. Estrutura de Pastas

```text
app/
  README.md
  documentacao.md
  backend/
    .env.example
    package.json
    server.js
    data/
      db.json
  frontend/
    assets/
      css/
        main.css
      js/
        drinks.js
    pages/
      auth.html
      drinks.html
```

---

## 3. Requisitos

- Node.js 18+
- npm

---

## 4. Instalação e Execução Local

1. Acesse a pasta do backend:
```bash
cd backend
```

2. Instale dependências:
```bash
npm install
```

3. Inicie o servidor:
```bash
npm start
```

4. Abra no navegador:
```text
http://localhost:3000
```

Ao acessar `/`, o backend redireciona para:
- `frontend/pages/auth.html`

---

## 5. Variáveis de Ambiente

Arquivo de exemplo:
- `backend/.env.example`

Variáveis disponíveis:
- `PORT`: porta HTTP do servidor (padrão: `3000`)
- `JWT_SECRET`: segredo para assinatura de tokens JWT

### Comportamento atual
No código, caso variáveis não sejam definidas, o backend usa fallback:
- `PORT || 3000`
- `JWT_SECRET || 'dev_local_secret_change_me'`

### Recomendação para produção
- definir `JWT_SECRET` forte e único;
- não usar segredos padrão.

---

## 6. Backend

### 6.1 Dependências
No `backend/package.json`:
- `express`: servidor HTTP e roteamento
- `cors`: habilita requisições cross-origin
- `bcryptjs`: hash e validação de senha
- `jsonwebtoken`: geração/validação de token JWT

### 6.2 Arquitetura (`backend/server.js`)

Principais pontos:
- inicializa Express e middlewares (`cors`, `express.json`, `express.static`);
- serve arquivos estáticos da raiz do projeto;
- cria e mantém o arquivo de banco local (`db.json`) automaticamente;
- valida autenticação com middleware `auth` via header `Authorization: Bearer <token>`;
- expõe endpoints de saúde, autenticação e CRUD de drinks.

### 6.3 Persistência Local

O banco local fica em:
- `backend/data/db.json`

Estrutura principal:
- `users[]`
- `drinks[]`

Leitura/escrita:
- `readDb()`: lê JSON do arquivo
- `writeDb(db)`: sobrescreve arquivo com JSON formatado
- `ensureDb()`: garante pasta/arquivo existentes

> Observação: é uma persistência simples e adequada para estudo/local. Em produção, recomenda-se banco de dados gerenciado.

---

## 7. Frontend

### 7.1 Páginas

- `frontend/pages/auth.html`
  - tela com abas de Login e Cadastro;
  - envia credenciais para backend;
  - salva token no `localStorage`;
  - redireciona para `drinks.html` quando autenticado.

- `frontend/pages/drinks.html`
  - dashboard de receitas salvas;
  - modal para criar/editar drink;
  - botões para editar, excluir e sair.

### 7.2 Estilos

- `frontend/assets/css/main.css`
  - tema escuro;
  - estilos de cards, formulários, modal e lista de drinks;
  - classes utilitárias para ações de salvar/editar/excluir.

### 7.3 Lógica de Interface e Regras

- `frontend/assets/js/drinks.js`

Funcionalidades principais:
- cálculo de custo e preço sugerido;
- carregamento de drinks do backend;
- criação, edição e exclusão via API;
- controle de sessão por token;
- inventário local de bebidas (via `localStorage`) para autocomplete.

Função matemática central:
- `calculateSuggestedPriceMath(totalCost, cmvPercentage)`

Fórmula utilizada:

$$
preco\_sugerido = \frac{custo\_total}{cmv/100}
$$

Exemplo:
- custo total = R$ 10,00
- CMV = 25%
- preço sugerido = $10 / 0.25 = 40$

---

## 8. Fluxo de Uso da Aplicação

1. Usuário acessa `/`.
2. Backend redireciona para tela de autenticação.
3. Usuário faz cadastro ou login.
4. Frontend recebe `token` e salva no `localStorage`.
5. Usuário acessa a tela de drinks.
6. Frontend envia token no header `Authorization`.
7. Backend filtra drinks pelo `userId` do token.
8. Usuário cria/edita/exclui receitas.

---

## 9. API REST - Endpoints

Base URL local:
- `http://localhost:3000`

### 9.1 Saúde

#### `GET /api/health`
Retorna status do servidor.

Resposta (200):
```json
{
  "ok": true,
  "message": "Servidor online."
}
```

---

### 9.2 Autenticação

#### `POST /api/auth/register`
Cria usuário e retorna token.

Body:
```json
{
  "email": "usuario@email.com",
  "password": "123456"
}
```

Regras:
- email obrigatório
- senha obrigatória
- senha com mínimo de 6 caracteres
- email único

Resposta (201):
```json
{
  "token": "jwt_token"
}
```

Erros comuns:
- `400`: dados inválidos
- `409`: email já cadastrado
- `500`: erro interno

#### `POST /api/auth/login`
Autentica usuário existente e retorna token.

Body:
```json
{
  "email": "usuario@email.com",
  "password": "123456"
}
```

Resposta (200):
```json
{
  "token": "jwt_token"
}
```

Erros comuns:
- `400`: campos ausentes
- `401`: credenciais inválidas
- `500`: erro interno

---

### 9.3 Drinks (Autenticado)

Todos os endpoints abaixo exigem header:
```http
Authorization: Bearer <token>
```

#### `GET /api/drinks`
Lista drinks do usuário autenticado.

Resposta (200):
```json
[
  {
    "id": "uuid",
    "userId": "uuid",
    "name": "Negroni",
    "cost": 14.6,
    "price": 41.71,
    "cmv": 35,
    "spirits": [],
    "ingredients": [],
    "date": "2026-04-23T18:35:17.309Z"
  }
]
```

#### `POST /api/drinks`
Cria um novo drink.

Body:
```json
{
  "name": "Negroni",
  "cost": 14.6,
  "price": 41.71,
  "cmv": 35,
  "spirits": [
    { "name": "Gin", "price": "120", "volume": "750", "dose": "30" }
  ],
  "ingredients": [
    { "name": "Gelo", "cost": "3" }
  ],
  "date": "2026-04-23T18:35:17.309Z"
}
```

Regras:
- `name` obrigatório
- `cost` deve ser maior que 0

Resposta (201): objeto do drink criado

#### `PUT /api/drinks/:id`
Atualiza drink do usuário autenticado.

Body: mesmo formato de criação (campos atualizáveis).

Resposta (200): objeto atualizado

Erros comuns:
- `404`: drink não encontrado

#### `DELETE /api/drinks/:id`
Exclui drink do usuário autenticado.

Resposta (200):
```json
{
  "message": "Drink removido com sucesso."
}
```

Erros comuns:
- `404`: drink não encontrado

---

## 10. Modelo de Dados

### 10.1 Usuário

```json
{
  "id": "uuid",
  "email": "string",
  "passwordHash": "string",
  "createdAt": "ISODate"
}
```

### 10.2 Drink

```json
{
  "id": "uuid",
  "userId": "uuid",
  "name": "string",
  "cost": "number",
  "price": "number",
  "cmv": "number",
  "spirits": [
    {
      "name": "string",
      "price": "string|number",
      "volume": "string|number",
      "dose": "string|number"
    }
  ],
  "ingredients": [
    {
      "name": "string",
      "cost": "string|number"
    }
  ],
  "date": "ISODate"
}
```

---

## 11. Segurança e Limitações Atuais

### Pontos positivos
- senhas armazenadas com hash (`bcrypt`);
- rotas de drinks protegidas por JWT;
- cada usuário só acessa seus próprios drinks.

### Limitações
- banco em arquivo local não é ideal para concorrência/escala;
- fallback de segredo JWT deve ser evitado em produção;
- dados de inventário no frontend ficam em `localStorage`.

---

## 12. Deploy e Evolução Recomendada

Para ambiente de produção:
- migrar persistência para banco real (PostgreSQL, MongoDB etc.);
- configurar variáveis de ambiente seguras;
- restringir CORS para domínio oficial;
- adicionar logs estruturados e monitoramento;
- incluir testes automatizados (unitários e integração);
- considerar refresh token e política de logout mais robusta.

---

## 13. Scripts Disponíveis

No backend (`backend/package.json`):
- `npm start` -> inicia servidor (`node server.js`)
- `npm run dev` -> atualmente igual ao start (`node server.js`)

---

## 14. Troubleshooting Rápido

### Não abre no navegador
- confirme que o backend está rodando em `http://localhost:3000`.

### Erro de autenticação / sessão expirada
- faça logout e login novamente;
- confira se `JWT_SECRET` não foi alterado entre emissões de token.

### Erro ao salvar drink
- verifique se `name` está preenchido e se `cost > 0`.

### Falha ao escrever no banco local
- confirme permissão de escrita em `backend/data/`.

---

## 15. Resumo Final

Este projeto implementa um fluxo completo de autenticação e CRUD de drinks com cálculo de precificação baseado em CMV, utilizando stack simples para aprendizado e execução local. A base está pronta para evolução para ambientes de produção com troca da camada de persistência e fortalecimento de segurança operacional.
