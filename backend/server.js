require('dotenv').config();
const express = require('express');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const cors = require('cors');
const path = require('path');
const { createClient } = require('@supabase/supabase-js');

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'dev_local_secret_change_me';
const APP_ROOT = path.resolve(__dirname, '..');

const SUPABASE_URL = process.env.SUPABASE_URL;
const SUPABASE_KEY = process.env.SUPABASE_ANON_KEY;

if (!SUPABASE_URL || !SUPABASE_KEY) {
    console.error("ERRO CRÍTICO: SUPABASE_URL e SUPABASE_ANON_KEY não foram definidos nas variáveis de ambiente (.env).");
    process.exit(1);
}

const supabase = createClient(SUPABASE_URL, SUPABASE_KEY);

app.use(cors());
app.use(express.json());
app.use(express.static(APP_ROOT));

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

        const passwordHash = await bcrypt.hash(password, 10);

        const { data, error } = await supabase
            .from('users')
            .insert([{ email: String(email).toLowerCase(), password_hash: passwordHash }])
            .select();

        if (error) {
            console.error('Erro detalhado do Supabase:', error);
            if (error.code === '23505') { // Unique violation
                return res.status(409).json({ message: 'Este e-mail já está cadastrado.' });
            }
            return res.status(400).json({ message: 'Erro ao cadastrar. Verifique o terminal.' });
        }

        const user = data[0];
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

        const { data: users, error } = await supabase
            .from('users')
            .select('*')
            .eq('email', String(email).toLowerCase());

        if (error || !users || users.length === 0) {
            return res.status(401).json({ message: 'Credenciais inválidas.' });
        }

        const user = users[0];
        const validPassword = await bcrypt.compare(password, user.password_hash);
        if (!validPassword) {
            return res.status(401).json({ message: 'Credenciais inválidas.' });
        }

        const token = jwt.sign({ id: user.id, email: user.email }, JWT_SECRET, { expiresIn: '7d' });
        return res.json({ token });
    } catch (error) {
        return res.status(500).json({ message: 'Erro interno ao fazer login.' });
    }
});

app.get('/api/drinks', auth, async (req, res) => {
    try {
        const { data: drinks, error } = await supabase
            .from('drinks')
            .select('*')
            .eq('user_id', req.user.id)
            .order('date', { ascending: false });

        if (error) throw error;

        return res.json(drinks);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao buscar drinks.' });
    }
});

app.post('/api/drinks', auth, async (req, res) => {
    try {
        const { name, cost, price, cmv, spirits, ingredients, date } = req.body;

        if (!name || Number(cost) <= 0) {
            return res.status(400).json({ message: 'Nome e custo válido são obrigatórios.' });
        }

        const newDrink = {
            user_id: req.user.id,
            name: String(name).trim(),
            cost: Number(cost) || 0,
            price: Number(price) || 0,
            cmv: Number(cmv) || 0,
            spirits: Array.isArray(spirits) ? spirits : [],
            ingredients: Array.isArray(ingredients) ? ingredients : [],
            date: date || new Date().toISOString()
        };

        const { data, error } = await supabase
            .from('drinks')
            .insert([newDrink])
            .select();

        if (error) throw error;

        return res.status(201).json(data[0]);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao salvar drink.' });
    }
});

app.put('/api/drinks/:id', auth, async (req, res) => {
    try {
        const { id } = req.params;

        // Simplified for partial updates, or explicit mapping
        const updateData = {
            name: String(req.body.name).trim(),
            cost: Number(req.body.cost) || 0,
            price: Number(req.body.price) || 0,
            cmv: Number(req.body.cmv) || 0,
            spirits: Array.isArray(req.body.spirits) ? req.body.spirits : undefined,
            ingredients: Array.isArray(req.body.ingredients) ? req.body.ingredients : undefined,
            date: req.body.date
        };

        // Remove undefined keys
        Object.keys(updateData).forEach(key => updateData[key] === undefined && delete updateData[key]);

        const { data, error } = await supabase
            .from('drinks')
            .update(updateData)
            .eq('id', id)
            .eq('user_id', req.user.id)
            .select();

        if (error || !data || data.length === 0) {
            return res.status(404).json({ message: 'Drink não encontrado ou erro ao atualizar.' });
        }

        return res.json(data[0]);
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao atualizar drink.' });
    }
});

app.delete('/api/drinks/:id', auth, async (req, res) => {
    try {
        const { id } = req.params;

        const { error } = await supabase
            .from('drinks')
            .delete()
            .eq('id', id)
            .eq('user_id', req.user.id);

        if (error) {
            return res.status(404).json({ message: 'Drink não encontrado ou erro ao excluir.' });
        }

        return res.json({ message: 'Drink removido com sucesso.' });
    } catch (error) {
        return res.status(500).json({ message: 'Erro ao excluir drink.' });
    }
});

app.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
});