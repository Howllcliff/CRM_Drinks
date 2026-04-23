const express = require('express');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const cors = require('cors');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'dev_local_secret_change_me';
const APP_ROOT = path.resolve(__dirname, '..');
const DB_PATH = path.join(__dirname, 'data', 'db.json');

app.use(cors());
app.use(express.json());
app.use(express.static(APP_ROOT));

const ensureDb = () => {
    const dbDir = path.dirname(DB_PATH);
    if (!fs.existsSync(dbDir)) {
        fs.mkdirSync(dbDir, { recursive: true });
    }

    if (!fs.existsSync(DB_PATH)) {
        const initial = { users: [], drinks: [] };
        fs.writeFileSync(DB_PATH, JSON.stringify(initial, null, 2));
    }
};

const readDb = () => {
    ensureDb();
    return JSON.parse(fs.readFileSync(DB_PATH, 'utf-8'));
};

const writeDb = (db) => {
    fs.writeFileSync(DB_PATH, JSON.stringify(db, null, 2));
};

const auth = (req, res, next) => {
    const authHeader = req.header('Authorization') || '';
    const token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : authHeader;

    if (!token) {
        return res.status(401).json({ message: 'Acesso negado. Token ausente.' });
    }

    try {
        const verified = jwt.verify(token, JWT_SECRET);
        req.user = verified;
        return next();
    } catch (error) {
        return res.status(401).json({ message: 'Token inválido ou expirado.' });
    }
};

app.get('/', (req, res) => {
    res.redirect('/frontend/pages/auth.html');
});

app.get('/api/health', (req, res) => {
    res.json({ ok: true, message: 'Servidor online.' });
});

app.post('/api/auth/register', async (req, res) => {
    try {
        const { email, password } = req.body;

        if (!email || !password) {
            return res.status(400).json({ message: 'E-mail e senha são obrigatórios.' });
        }

        if (password.length < 6) {
            return res.status(400).json({ message: 'A senha deve ter pelo menos 6 caracteres.' });
        }

        const db = readDb();
        const existing = db.users.find((u) => u.email.toLowerCase() === String(email).toLowerCase());
        if (existing) {
            return res.status(409).json({ message: 'Este e-mail já está cadastrado.' });
        }

        const passwordHash = await bcrypt.hash(password, 10);
        const user = {
            id: crypto.randomUUID(),
            email: String(email).toLowerCase(),
            passwordHash,
            createdAt: new Date().toISOString()
        };

        db.users.push(user);
        writeDb(db);

        const token = jwt.sign({ id: user.id, email: user.email }, JWT_SECRET, { expiresIn: '7d' });
        return res.status(201).json({ token });
    } catch (error) {
        return res.status(500).json({ message: 'Erro interno ao cadastrar usuário.' });
    }
});

app.post('/api/auth/login', async (req, res) => {
    try {
        const { email, password } = req.body;

        if (!email || !password) {
            return res.status(400).json({ message: 'E-mail e senha são obrigatórios.' });
        }

        const db = readDb();
        const user = db.users.find((u) => u.email.toLowerCase() === String(email).toLowerCase());
        if (!user) {
            return res.status(401).json({ message: 'Credenciais inválidas.' });
        }

        const validPassword = await bcrypt.compare(password, user.passwordHash);
        if (!validPassword) {
            return res.status(401).json({ message: 'Credenciais inválidas.' });
        }

        const token = jwt.sign({ id: user.id, email: user.email }, JWT_SECRET, { expiresIn: '7d' });
        return res.json({ token });
    } catch (error) {
        return res.status(500).json({ message: 'Erro interno ao fazer login.' });
    }
});

app.get('/api/drinks', auth, (req, res) => {
    try {
        const db = readDb();
        const drinks = db.drinks
            .filter((drink) => drink.userId === req.user.id)
            .sort((a, b) => new Date(b.date) - new Date(a.date));

        return res.json(drinks);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao buscar drinks.' });
    }
});

app.post('/api/drinks', auth, (req, res) => {
    try {
        const { name, cost, price, cmv, spirits, ingredients, date } = req.body;

        if (!name || Number(cost) <= 0) {
            return res.status(400).json({ message: 'Nome e custo válido são obrigatórios.' });
        }

        const db = readDb();
        const newDrink = {
            id: crypto.randomUUID(),
            userId: req.user.id,
            name: String(name).trim(),
            cost: Number(cost) || 0,
            price: Number(price) || 0,
            cmv: Number(cmv) || 0,
            spirits: Array.isArray(spirits) ? spirits : [],
            ingredients: Array.isArray(ingredients) ? ingredients : [],
            date: date || new Date().toISOString()
        };

        db.drinks.push(newDrink);
        writeDb(db);

        return res.status(201).json(newDrink);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao salvar drink.' });
    }
});

app.put('/api/drinks/:id', auth, (req, res) => {
    try {
        const { id } = req.params;
        const db = readDb();
        const index = db.drinks.findIndex((drink) => drink.id === id && drink.userId === req.user.id);

        if (index < 0) {
            return res.status(404).json({ message: 'Drink não encontrado.' });
        }

        const current = db.drinks[index];
        const updatedDrink = {
            ...current,
            name: String(req.body.name || current.name).trim(),
            cost: Number(req.body.cost) || 0,
            price: Number(req.body.price) || 0,
            cmv: Number(req.body.cmv) || 0,
            spirits: Array.isArray(req.body.spirits) ? req.body.spirits : current.spirits,
            ingredients: Array.isArray(req.body.ingredients) ? req.body.ingredients : current.ingredients,
            date: req.body.date || current.date
        };

        db.drinks[index] = updatedDrink;
        writeDb(db);

        return res.json(updatedDrink);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao atualizar drink.' });
    }
});

app.delete('/api/drinks/:id', auth, (req, res) => {
    try {
        const { id } = req.params;
        const db = readDb();
        const index = db.drinks.findIndex((drink) => drink.id === id && drink.userId === req.user.id);

        if (index < 0) {
            return res.status(404).json({ message: 'Drink não encontrado.' });
        }

        db.drinks.splice(index, 1);
        writeDb(db);

        return res.json({ message: 'Drink removido com sucesso.' });
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao excluir drink.' });
    }
});

app.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
});
