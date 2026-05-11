content = """# Sistema de Receituário e Precificação de Drinks 🍸

Este é um sistema completo de gestão para profissionais de coquetelaria, permitindo o registo detalhado de receitas, cálculo de custos de produção e definição de preços de venda com base na meta de CMV (Custo de Mercadoria Vendida).

## 🚀 Funcionalidades Principal

* **Autenticação Segura**: Registo e login de utilizadores com senhas criptografadas (Bcrypt) e sessões via JWT (JSON Web Tokens).
* **Calculadora de CMV em Tempo Real**: Cálculo dinâmico do custo por dose com base no valor da garrafa e volume utilizado.
* **Gestão de Receituário (CRUD)**: Criação, listagem, edição e exclusão de drinks personalizados.
* **Memória de Inventário**: O sistema "aprende" os preços das bebidas base inseridas, facilitando novos registos através de preenchimento automático.
* **Design Responsivo**: Interface optimizada para uso em computadores e dispositivos móveis (tablets/smartphones).

## 🛠️ Tecnologias Utilizadas

### Frontend
* **HTML5 & CSS3**: Estrutura e estilização moderna com uso de variáveis CSS e flexbox/grid.
* **JavaScript (Vanilla)**: Lógica de interface, manipulação do DOM e consumo de API sem dependências externas.
* **LocalStorage**: Persistência local para cache de inventário.

### Backend
* **Node.js & Express**: Servidor e roteamento de API RESTful.
* **Supabase (PostgreSQL)**: Persistência de dados em nuvem e gestão de utilizadores.
* **JWT & Bcrypt**: Camadas de segurança para autenticação e protecção de dados sensíveis.

## 📦 Como Instalar e Executar

### Pré-requisitos
* [Node.js](https://nodejs.org/) instalado.
* Conta no [Supabase](https://supabase.com/) com um projecto configurado.

### Passo a Passo

1.  **Clonar o repositório:**
    ```bash
    git clone <url-do-repositorio>
    cd <pasta-do-projecto>
    ```

2.  **Instalar as dependências:**
    ```bash
    npm install
    ```


4.  **Iniciar o Servidor:**
    ```bash
    npm start
    ```
    O servidor estará disponível em `http://localhost:3000`.

## 📖 Estrutura de Pastas

```text
├── assets/             # Ficheiros estáticos (CSS, JS do Frontend)
│   ├── css/            # Estilização global e específica
│   └── js/             # Lógica do frontend (drinks.js, login.js)
├── pages/              # Páginas HTML (auth.html, drinks.html)
├── server.js           # Servidor backend (Node.js/Express)
└── .env                # Variáveis de ambiente (não incluído no git)