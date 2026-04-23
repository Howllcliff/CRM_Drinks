// Função pura isolada
const calculateSuggestedPriceMath = (totalCost, cmvPercentage) => {
    if (cmvPercentage <= 0) return 0;
    return totalCost / (cmvPercentage / 100);
};

if (typeof document !== 'undefined') {
    const API_BASE_URL = 'http://localhost:3000';

    const spiritsContainer = document.getElementById('spirits-container');
    const ingredientsContainer = document.getElementById('ingredients-container');
    const savedContainer = document.getElementById('saved-drinks-container');

    let savedDrinks = [];
    let currentTotalCost = 0;
    let currentSuggestedPrice = 0;
    let editIndex = -1;

    const formatBRL = (value) => new Intl.NumberFormat('pt-BR', { style: 'currency', currency: 'BRL' }).format(value);

    const escapeHtml = (text) => {
        const map = {
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#039;'
        };
        return String(text || '').replace(/[&<>"']/g, (m) => map[m]);
    };

    const getToken = () => localStorage.getItem('token');

    const redirectToAuth = () => {
        localStorage.removeItem('token');
        window.location.href = './auth.html';
    };

    const apiRequest = async (path, options = {}) => {
        const token = getToken();
        if (!token) {
            redirectToAuth();
            return null;
        }

        const response = await fetch(`${API_BASE_URL}${path}`, {
            ...options,
            headers: {
                'Content-Type': 'application/json',
                Authorization: `Bearer ${token}`,
                ...(options.headers || {})
            }
        });

        if (response.status === 401) {
            alert('Sessão expirada. Faça login novamente.');
            redirectToAuth();
            return null;
        }

        return response;
    };

    // INVENTÁRIO (Reutilização de bebidas)
    const updateInventoryDatalist = () => {
        const inventory = JSON.parse(localStorage.getItem('drinkInventory')) || {};
        const datalist = document.getElementById('inventory-list');
        datalist.innerHTML = '';
        for (const spiritName in inventory) {
            const option = document.createElement('option');
            option.value = spiritName;
            datalist.appendChild(option);
        }
    };

    // Auto-preencher dados se a bebida existir no inventário
    const handleSpiritNameChange = (event) => {
        const name = event.target.value.trim();
        const inventory = JSON.parse(localStorage.getItem('drinkInventory')) || {};

        if (inventory[name]) {
            const row = event.target.closest('.spirit-row');
            row.querySelector('.spirit-price').value = inventory[name].price;
            row.querySelector('.spirit-vol').value = inventory[name].volume;
            calculateAll();
        }
    };

    const addSpiritRow = (name = '', price = '', volume = '', dose = '') => {
        const div = document.createElement('div');
        div.className = 'item-row spirit-row';
        div.innerHTML = `
            <button class="btn-remove" onclick="this.parentElement.remove(); calculateAll();">Remover</button>
            <div class="form-group" style="margin-bottom: 0;">
                <label>Ingrediente</label>
                <input type="text" class="spirit-name" list="inventory-list" placeholder="Ex: Vodka, Gin..." value="${name}">
            </div>
            <div class="item-grid-3">
                <div>
                    <label>Garrafa (R$)</label>
                    <input type="number" class="calc-trigger spirit-price" placeholder="0.00" min="0" step="0.01" value="${price}">
                </div>
                <div>
                    <label>Vol. (ml)</label>
                    <input type="number" class="calc-trigger spirit-vol" placeholder="750" min="1" value="${volume}">
                </div>
                <div>
                    <label>Dose (ml)</label>
                    <input type="number" class="calc-trigger spirit-dose" placeholder="50" min="0" step="1" value="${dose}">
                </div>
            </div>
        `;

        // Adiciona evento para buscar no inventário ao digitar o nome
        div.querySelector('.spirit-name').addEventListener('input', handleSpiritNameChange);
        spiritsContainer.appendChild(div);
    };

    const addIngredientRow = (name = '', cost = '') => {
        const div = document.createElement('div');
        div.className = 'item-row ingredient-row';
        div.innerHTML = `
            <button class="btn-remove" onclick="this.parentElement.remove(); calculateAll();">Remover</button>
            <div class="item-grid-2">
                <div>
                    <label>Insumo Extra</label>
                    <input type="text" class="ingredient-name" placeholder="Descrição" value="${name}">
                </div>
                <div>
                    <label>Custo (R$)</label>
                    <input type="number" class="calc-trigger ingredient-cost" placeholder="0.00" min="0" step="0.01" value="${cost}">
                </div>
            </div>
        `;
        ingredientsContainer.appendChild(div);
    };

    window.calculateAll = () => {
        let totalLiquidCost = 0;
        let totalExtraCost = 0;

        document.querySelectorAll('.spirit-row').forEach(row => {
            const price = parseFloat(row.querySelector('.spirit-price').value) || 0;
            const volume = parseFloat(row.querySelector('.spirit-vol').value) || 0;
            const dose = parseFloat(row.querySelector('.spirit-dose').value) || 0;
            if (volume > 0) totalLiquidCost += (price / volume) * dose;
        });

        document.querySelectorAll('.ingredient-row').forEach(row => {
            const cost = parseFloat(row.querySelector('.ingredient-cost').value) || 0;
            totalExtraCost += cost;
        });

        currentTotalCost = totalLiquidCost + totalExtraCost;
        const cmv = parseFloat(document.getElementById('cmv-input').value) || 0;

        currentSuggestedPrice = calculateSuggestedPriceMath(currentTotalCost, cmv);

        document.getElementById('res-total-cost').textContent = formatBRL(currentTotalCost);
        document.getElementById('res-suggested-price').textContent = formatBRL(currentSuggestedPrice);
    };

    const renderSavedDrinks = () => {
        savedContainer.innerHTML = '';

        if (savedDrinks.length === 0) {
            savedContainer.innerHTML = '<div class="empty-state">Nenhum drink salvo ainda. Clique em "+ Novo Drink".</div>';
            return;
        }

        savedDrinks.forEach((drink, index) => {
            const div = document.createElement('div');
            div.className = 'saved-drink-item';
            div.innerHTML = `
                <div class="saved-drink-info">
                    <h4>${escapeHtml(drink.name)}</h4>
                    <p>Custo: <strong>${formatBRL(drink.cost)}</strong> | Venda sugerida: <strong>${formatBRL(drink.price)}</strong> (CMV: ${drink.cmv}%)</p>
                </div>
                <div class="drink-actions">
                    <button class="btn-edit-saved" onclick="editDrink(${index})">Editar</button>
                    <button class="btn-delete-saved" onclick="deleteDrink(${index})">Excluir</button>
                </div>
            `;
            savedContainer.appendChild(div);
        });
    };

    const loadSavedDrinks = async () => {
        try {
            const response = await apiRequest('/api/drinks', { method: 'GET' });
            if (!response) return;

            if (!response.ok) {
                const data = await response.json();
                throw new Error(data.message || 'Falha ao carregar drinks.');
            }

            savedDrinks = await response.json();
            renderSavedDrinks();
        } catch (error) {
            alert(error.message || 'Erro ao carregar drinks do servidor.');
        }
    };

    const saveDrink = async () => {
        const nameInput = document.getElementById('drink-name');
        const drinkName = nameInput.value.trim();
        const cmv = parseFloat(document.getElementById('cmv-input').value) || 0;

        if (!drinkName) {
            alert("Por favor, digite o nome do drink antes de salvar!");
            return;
        }
        if (currentTotalCost === 0) {
            alert("Adicione insumos para calcular o custo antes de salvar!");
            return;
        }

        // Coleta os dados para poder editá-los futuramente
        const spiritsData = [];
        let inventory = JSON.parse(localStorage.getItem('drinkInventory')) || {};

        document.querySelectorAll('.spirit-row').forEach(row => {
            const name = row.querySelector('.spirit-name').value.trim();
            const price = row.querySelector('.spirit-price').value;
            const volume = row.querySelector('.spirit-vol').value;
            const dose = row.querySelector('.spirit-dose').value;

            if (name) {
                spiritsData.push({ name, price, volume, dose });
                // Salva ou atualiza a bebida no inventário para autocompletar da próxima vez
                if (price && volume) inventory[name] = { price, volume };
            }
        });
        localStorage.setItem('drinkInventory', JSON.stringify(inventory));
        updateInventoryDatalist(); // Atualiza o datalist

        const ingredientsData = [];
        document.querySelectorAll('.ingredient-row').forEach(row => {
            const name = row.querySelector('.ingredient-name').value.trim();
            const cost = row.querySelector('.ingredient-cost').value;
            if (name || cost) ingredientsData.push({ name, cost });
        });

        const newDrink = {
            name: drinkName,
            cost: currentTotalCost,
            price: currentSuggestedPrice,
            cmv: cmv,
            spirits: spiritsData,
            ingredients: ingredientsData,
            date: new Date().toISOString()
        };

        try {
            const isEditing = editIndex >= 0;
            const drinkToEdit = isEditing ? savedDrinks[editIndex] : null;

            let response;
            if (isEditing && drinkToEdit && drinkToEdit.id) {
                response = await apiRequest(`/api/drinks/${drinkToEdit.id}`, {
                    method: 'PUT',
                    body: JSON.stringify(newDrink)
                });
            } else {
                response = await apiRequest('/api/drinks', {
                    method: 'POST',
                    body: JSON.stringify(newDrink)
                });
            }

            if (!response) return;

            const data = await response.json();
            if (!response.ok) {
                throw new Error(data.message || 'Falha ao salvar drink.');
            }

            alert(`Drink "${drinkName}" ${isEditing ? 'atualizado' : 'salvo'} com sucesso!`);

            // Fecha o modal via CSS (limpando o hash da URL)
            window.location.hash = '';
            await loadSavedDrinks();
            clearForm();
        } catch (error) {
            alert(error.message || 'Erro ao salvar drink no servidor.');
        }
    };

    // Prepara o modal para criar um NOVO drink
    window.prepareNewDrink = () => {
        editIndex = -1;
        document.getElementById('modal-title').innerText = "Cadastrar Novo Drink";
        clearForm();
    };

    // Limpa o formulário do modal
    window.clearForm = () => {
        document.getElementById('drink-name').value = '';
        document.getElementById('cmv-input').value = '25';
        spiritsContainer.innerHTML = '';
        ingredientsContainer.innerHTML = '';
        addSpiritRow();
        addIngredientRow();
        calculateAll();
    };

    // Abre o modal preenchido com os dados do drink para edição
    window.editDrink = (index) => {
        const drink = savedDrinks[index];
        if (!drink) return;

        editIndex = index;
        document.getElementById('modal-title').innerText = `Editar Drink: ${drink.name}`;

        document.getElementById('drink-name').value = drink.name;
        document.getElementById('cmv-input').value = drink.cmv || 25;

        spiritsContainer.innerHTML = '';
        if (drink.spirits && drink.spirits.length > 0) {
            drink.spirits.forEach(s => addSpiritRow(s.name, s.price, s.volume, s.dose));
        } else {
            addSpiritRow();
        }

        ingredientsContainer.innerHTML = '';
        if (drink.ingredients && drink.ingredients.length > 0) {
            drink.ingredients.forEach(i => addIngredientRow(i.name, i.cost));
        } else {
            addIngredientRow();
        }

        calculateAll();
        // Abre o modal ativando o target no CSS
        window.location.hash = '#modal-cadastro';
    };

    window.deleteDrink = async (index) => {
        if (confirm("Tem certeza que deseja excluir este drink?")) {
            const drink = savedDrinks[index];
            if (!drink || !drink.id) return;

            try {
                const response = await apiRequest(`/api/drinks/${drink.id}`, { method: 'DELETE' });
                if (!response) return;

                const data = await response.json();
                if (!response.ok) {
                    throw new Error(data.message || 'Falha ao excluir drink.');
                }

                await loadSavedDrinks();
            } catch (error) {
                alert(error.message || 'Erro ao excluir drink no servidor.');
            }
        }
    };

    window.logout = () => {
        redirectToAuth();
    };

    // Event Listeners
    document.getElementById('btn-add-spirit').addEventListener('click', (event) => {
        event.preventDefault();
        addSpiritRow();
    });
    document.getElementById('btn-add-ingredient').addEventListener('click', (event) => {
        event.preventDefault();
        addIngredientRow();
    });
    document.getElementById('btn-save-drink').addEventListener('click', saveDrink);

    document.getElementById('calculator-form').addEventListener('input', (e) => {
        if (e.target.classList.contains('calc-trigger')) {
            calculateAll();
        }
    });

    // Inicialização
    const init = async () => {
        const token = getToken();
        if (!token) {
            redirectToAuth();
            return;
        }

        updateInventoryDatalist();
        clearForm();
        await loadSavedDrinks();
    };

    init();
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = { calculateSuggestedPriceMath };
}