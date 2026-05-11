## 1. Introdução
Este documento detalha as especificações técnicas, arquiteturais e funcionais do Sistema de Drinks, uma plataforma web projetada para o cadastro de receitas, controle de insumos e cálculo automatizado de precificação baseado em metas de CMV (Custo de Mercadoria Vendida).

O sistema foi concebido para atender às necessidades de profissionais de mixologia e gestão de bares, permitindo uma visão clara da margem de lucro por produto.

---

## 2. Tecnologias Utilizadas
A solução adota uma stack moderna e escalável, focada em performance e segurança:

| Camada | Tecnologia | Descrição |
| :--- | :--- | :--- |
| **Frontend** | HTML5 / CSS3 / JS Vanilla | Interface responsiva e lógica de cliente sem dependências pesadas. |
| **Backend** | Node.js / Express | Servidor para processamento de regras de negócio e roteamento de API. |
| **Banco de Dados** | Supabase (PostgreSQL) | Persistência de dados relacional com alta disponibilidade. |
| **Segurança** | JWT & Bcrypt | Autenticação baseada em tokens e criptografia de senhas. |

---

## 3. Arquitetura do Sistema
O sistema segue o modelo **Cliente-Servidor**. O frontend comunica-se com o backend através de uma API RESTful, utilizando cabeçalhos de autorização Bearer para rotas protegidas.

### 3.1. Estrutura de Diretórios
- `assets/css/`: Folhas de estilo para layout global e login.
- `assets/js/`: Lógica de autenticação e manipulação de drinks.
- `pages/`: Interfaces do usuário (`auth.html` e `drinks.html`).
- `server/`: Código do servidor Node.js (`server.js`).

---

## 4. Requisitos do Sistema

### 4.1. Requisitos Funcionais (RF)
| ID | Descrição |
| :--- | :--- |
| **RF-01** | O sistema deve permitir o cadastro e login de usuários com validação de credenciais. |
| **RF-02** | O usuário autenticado deve ser capaz de criar, editar e excluir drinks de seu próprio receituário. |
| **RF-03** | O sistema deve calcular o custo total do drink somando insumos e doses de bebidas. |
| **RF-04** | O sistema deve sugerir um preço de venda com base na meta de CMV informada. |
| **RF-05** | O sistema deve oferecer um inventário local (datalist) para autocompletar preços de bebidas já cadastradas. |

### 4.2. Requisitos Não Funcionais (RNF)
- **Segurança**: Senhas devem ser armazenadas apenas como hashes Bcrypt.
- **Sessão**: Autenticação via JSON Web Token (JWT) com expiração de 7 dias.
- **Responsividade**: A interface deve ser adaptável a dispositivos móveis através de media queries.

---

## 5. Modelo de Dados
A persistência é realizada no Supabase. Abaixo estão as definições lógicas das entidades:

### Tabela: `users`
- `id`: UUID (Chave Primária)
- `email`: String (Único, formato minúsculo)
- `password_hash`: String (Criptografada)

### Tabela: `drinks`
- `id`: UUID (Chave Primária)
- `user_id`: UUID (Chave Estrangeira para `users`)
- `name`: String (Nome do drink)
- `cost`: Numeric (Custo total calculado)
- `price`: Numeric (Preço de venda sugerido)
- `cmv`: Numeric (Porcentagem de CMV aplicada)
- `spirits`: JSONB (Array de objetos contendo nome, preço, volume e dose)
- `ingredients`: JSONB (Array de objetos contendo nome e custo fixo)
- `date`: Timestamp

---

## 6. Lógica de Precificação
A regra de negócio central reside na função de cálculo de CMV. O preço de venda é determinado pela fórmula:

`Preço Sugerido = Custo Total / (Meta de CMV / 100)`

Onde o **Custo Total** é a soma de:
1. **Custo do Líquido**: `(Preço da Garrafa / Volume Total) * Dose Utilizada`.
2. **Custo de Insumos**: Soma dos custos fixos de guarnições e gelo.

---

## 7. Endpoints da API (REST)

### Autenticação
- `POST /api/auth/register`: Cadastro de novo usuário.
- `POST /api/auth/login`: Autenticação e geração de token.

### Drinks (Requer Header: `Authorization: Bearer <token>`)
- `GET /api/drinks`: Lista todos os drinks do usuário logado.
- `POST /api/drinks`: Cadastra um novo drink.
- `PUT /api/drinks/:id`: Atualiza um drink existente.
- `DELETE /api/drinks/:id`: Remove um drink do sistema.

---

## 8. Considerações de Segurança
1. **Middleware de Autenticação**: Todas as rotas de manipulação de dados passam por uma função que verifica a validade do token JWT.
2. **Prevenção de XSS**: A interface realiza o escape de strings antes de renderizá-las no dashboard.
3. **Isolamento de Dados**: Toda consulta ao banco de dados inclui o filtro `user_id`, garantindo que um usuário nunca acesse dados de terceiros.
"""

with open("documentacao.md", "w", encoding="utf-8") as f:
    f.write(md_content)