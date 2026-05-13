# 🍸 Calculadora de Ficha Técnica de Drinks

[![Versão](https://img.shields.io/badge/versão-0.1.0-blue.svg)](https://semver.org/)

Uma aplicação de interface gráfica moderna focada na eficiência operacional do setor de bar. Esta ferramenta permite a criação de fichas técnicas padronizadas, cálculo automático de custos e precificação baseada na meta de **CMV (Custo da Mercadoria Vendida)**, além de manter um receituário digital salvo localmente.


## O Projeto

A gestão de custos em um bar exige precisão. Este sistema foi desenvolvido para resolver o problema de cálculos manuais e erros na hora de criar novos coquetéis ou atualizar preços. Através de uma interface web simples e de tema escuro (para conforto visual em ambientes de pouca luz), a ferramenta entrega agilidade e previsibilidade de lucros para o salão.

**Público-Alvo:** Bartenders, gestores de A&B (Alimentos e Bebidas), donos de bares e restaurantes que precisam de um controle financeiro rigoroso sobre suas receitas.

### Funcionalidades Principais
* **Adição Múltipla:** Inserção de valor da garrafa, volume e dose de líquidos variados.
* **Insumos Extras:** Inclusão de guarnições, xaropes e gelo com custos fixos.
* **Precificação Automática:** Cálculo do Preço Ideal de Venda focado em metas percentuais.
* **Receituário Integrado:** Armazenamento local contínuo via `LocalStorage`.

---

## As Fórmulas (Como o cálculo é feito)

A lógica matemática do sistema garante que a precificação seja exata e proteja a margem de lucro.

### Custo Total do Drink
O custo real soma o valor fracionado de todos os líquidos com os custos fixos dos extras:
$$Custo_{total} = \sum \left( \frac{Preço_{garrafa}}{Volume_{garrafa}} \times Dose_{ml} \right) + \sum Custo_{extras}$$

### Preço de Venda Sugerido
O sistema utiliza a fórmula de marcação (markup divisor) visando atingir uma meta de CMV:
$$Preço_{sugerido} = \frac{Custo_{total}}{\left( \frac{CMV_{meta}}{100} \right)}$$

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

## Estrutura de Pastas

```text
├── assets/             # Ficheiros estáticos (CSS, JS do Frontend)
│   ├── css/            # Estilização global e específica
│   └── js/             # Lógica do frontend (drinks.js, login.js)
├── pages/              # Páginas HTML (auth.html, drinks.html)
├── server.js           # Servidor backend (Node.js/Express)
└── .env                # Variáveis de ambiente (não incluído no git)
```

## Telas do sistema

Tela de Login
![Login](doc_src/Screenshot%202026-05-05%20183418.png)

Tela de Cadastros
![Cadastros exemplos](doc_src/Screenshot%202026-05-05%20183429.png)

Modal de Cadastro
![Modal de Cadastro](doc_src/Screenshot%202026-05-05%20183447.png)

Exemplo de Cadastro
![Exemplo 4](doc_src/Screenshot%202026-05-05%20183507.png)