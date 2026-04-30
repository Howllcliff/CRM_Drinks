# Sistema de Drinks em Python

Versão em Python do projeto original, mantendo a mesma API e a mesma persistência em arquivo JSON.

## Estrutura

```text
python/
  README.md
  .env.example
  requirements.txt
  backend/
    app.py
    data/
      db.json
```

O backend em Flask serve o frontend que já existe na pasta raiz do projeto:

- `../frontend/pages/auth.html`
- `../frontend/pages/drinks.html`
- `../frontend/assets/css/*`
- `../frontend/assets/js/*`

## Requisitos

- Python 3.14+
- pip

## Instalação

1. Entre na pasta `python`.
2. Crie e ative um ambiente virtual, se desejar.
3. Instale as dependências:

```bash
pip install -r requirements.txt
```

4. Copie `.env.example` para `.env` e ajuste as variáveis, se necessário.

## Execução

```bash
python backend/app.py
```

Depois abra:

- `http://localhost:3000`

## Endpoints

- `GET /api/health`
- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/drinks`
- `POST /api/drinks`
- `PUT /api/drinks/<id>`
- `DELETE /api/drinks/<id>`

## Observações

- O banco local continua em JSON.
- A autenticação usa JWT.
- O backend filtra drinks por usuário autenticado, igual à versão em Node.js.
