# Documento de Detalhamento de Requisitos (DDR)

## Projeto: Sistema de Gestão e Precificação de Drinks
#### Versão: 1.0

## 1. Visão Geral do Produto

O produto é uma aplicação web projetada para o cadastro, gerenciamento e precificação de receituários de drinks. O sistema visa solucionar a dificuldade de controle de custos (CMV) no setor de coquetelaria, permitindo que os usuários salvem a composição exata de suas bebidas e calculem automaticamente o preço sugerido de venda com base nos insumos e destilados.

## 2. Requisitos Funcionais (RF)
Os Requisitos Funcionais descrevem as ações específicas que o sistema deve ser capaz de executar.

__*RF01 - Gestão de Conta de Usuário:*__ O sistema deve permitir o registro de novos usuários utilizando um endereço de e-mail e uma senha.

__*RF02 - Autenticação e Sessão:*__ O sistema deve autenticar os usuários cadastrados e fornecer um token (JWT) para manutenção da sessão segura.

__*RF03 - CRUD de Receituário:*__ O sistema deve permitir que o usuário logado realize a criação, leitura (listagem), atualização e exclusão (CRUD) de seus drinks.

__*RF04 - Composição do Drink:*__ Durante o cadastro do drink, o sistema deve permitir a adição múltipla e dinâmica de "Bebidas Base" (volume, dose e valor da garrafa) e "Insumos Extras" (valor fixo).

__*RF05 - Cálculo Financeiro:*__ O sistema deve calcular, em tempo real, o custo total de produção do drink e o preço de venda sugerido com base em uma meta de porcentagem de CMV informada pelo usuário.

__*RF06 - Memória de Inventário:*__ O sistema deve armazenar localmente as informações de preço e volume das bebidas base cadastradas para preencher automaticamente os campos em cadastros futuros.

## 2.1. Critérios de Aceite (Cenários BDD para a Calculadora - RF05)
Para garantir a integridade da funcionalidade principal, aplicamos o padrão de comportamento esperado para a precificação:

Funcionalidade: Cálculo de Preço Sugerido por CMV

Cenário: Cálculo de um drink com destilados e insumos extras

__Dado__ que o usuário está na tela de "Cadastrar Novo Drink"

__E__ insere uma garrafa de Vodka que custa $80.00$ com volume de $1000$ml e utiliza uma dose de $50ml

__E__ insere um insumo extra (Gelo/Guarnição) com custo de $1.00$

__E__ define a Meta de CMV como $25

__Quando__ os valores são processados pela calculadora

__Então__ o sistema deve exibir o Custo Total de $5.00$

__E__ deve exibir o Preço Sugerido (Venda) de $20.00$

# 3. Regras de Negócio (RN)
As Regras de Negócio impõem restrições e diretrizes operacionais lógicas que o sistema deve respeitar incondicionalmente.

__RN01 - Isolamento de Dados__ (Multitenancy lógico): Um usuário logado só pode visualizar, editar ou excluir os drinks que pertencem ao seu próprio identificador (user_id). É estritamente proibido o acesso cruzado entre contas.

__RN02 - Validação de Senha:__ Para o cadastro de um novo usuário, a senha fornecida deve conter, obrigatoriamente, um comprimento mínimo de 6 caracteres.

__RN03 - Unicidade de Cadastro:__ Não é permitido o registro de duas contas com o mesmo endereço de e-mail.

__RN04 - Fórmula Matemática do Preço Sugerido:__ A estipulação do preço de venda deve seguir a fórmula de divisão direta: $Preco = \frac{CustoTotal}{\frac{CMV}{100}}$. Caso o CMV informado seja menor ou igual a zero, o preço sugerido deve retornar zero para evitar erros de divisão.

## 4. Requisitos Não Funcionais (RNF)

Os Requisitos Não Funcionais especificam os critérios que avaliam a operação do sistema, focando em tecnologias, segurança e usabilidade.

__*RNF01 - Stack Tecnológica de Backend:*__ A API deve ser desenvolvida em Node.js utilizando o framework Express.

__*RNF02 - Persistência de Dados:*__ O sistema deve utilizar o Supabase (PostgreSQL) como Sistema de Gerenciamento de Banco de Dados.

__*RNF03 - Segurança de Senhas:*__ Nenhuma senha pode ser armazenada em texto plano; todas devem ser convertidas em hash utilizando o algoritmo bcrypt com custo (salting) padrão.

__*RNF04 - Segurança de Sessão:*__ A comunicação entre o cliente e o servidor para áreas restritas deve ser protegida pela validação de um token JWT inserido no cabeçalho (Header) da requisição sob o esquema Bearer.

__*RNF05 - Responsividade da Interface:*__ A interface de usuário deve se adaptar a dispositivos móveis, alterando proporções e bordas quando a largura da tela for inferior a 520px.

__*RNF06 - Tratamento de Injeção e XSS:*__ O frontend deve realizar o escape de caracteres especiais HTML (como <, >, &, ", ') antes de renderizar os nomes dos drinks salvos na interface, mitigando ataques de Cross-Site Scripting (XSS).