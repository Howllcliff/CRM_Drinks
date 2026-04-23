# Sistema de Drinks - Execução Local

## Estrutura do projeto
```
app/
   backend/
      data/
         db.json
      server.js
      package.json
   frontend/
      pages/
         auth.html
         drinks.html
      assets/
         css/
            main.css
         js/
            drinks.js
```

## Requisitos
- Node.js 18+
- npm

## Como rodar localmente
1. Entre na pasta do backend:
   cd backend
2. Instale as dependências:
   npm install
3. Inicie o servidor:
   npm start
4. Abra no navegador:
   http://localhost:3000

## Fluxo
- Crie sua conta na tela de cadastro.
- Faça login.
- O sistema redireciona para o receituário.
- Drinks são salvos por usuário no arquivo local `backend/data/db.json`.

## Estrutura de API
- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/drinks` (autenticada)
- `POST /api/drinks` (autenticada)
- `PUT /api/drinks/:id` (autenticada)
- `DELETE /api/drinks/:id` (autenticada)
- `GET /api/health`

## Preparação para nuvem
1. Definir variáveis de ambiente:
   - `PORT`
   - `JWT_SECRET`
2. Trocar persistência local de arquivo (`backend/data/db.json`) por banco gerenciado (MongoDB Atlas, PostgreSQL, etc.).
3. Configurar CORS para seu domínio em produção.
4. Usar HTTPS e segredo forte para JWT.

## Observações
- O backend serve os arquivos estáticos da pasta raiz do projeto.
- A rota inicial (`/`) redireciona para `frontend/pages/auth.html`.
