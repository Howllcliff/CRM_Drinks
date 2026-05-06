
        const API_BASE_URL = 'http://localhost:3000';

        let mode = 'login';
        function toggleAuth(m) {
            mode = m;
            document.getElementById('auth-title').innerText = m === 'login' ? 'Entrar' : 'Criar Conta';
            document.getElementById('btn-tab-login').classList.toggle('active', m === 'login');
            document.getElementById('btn-tab-register').classList.toggle('active', m === 'register');
        }

        (function redirectIfAlreadyLoggedIn() {
            const token = localStorage.getItem('token');
            if (token) {
                window.location.href = './drinks.html';
            }
        })();

        document.getElementById('auth-form').onsubmit = async (e) => {
            e.preventDefault();
            const email = document.getElementById('email').value;
            const password = document.getElementById('password').value;

            try {
                const res = await fetch(`${API_BASE_URL}/api/auth/${mode}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ email, password })
                });

                const data = await res.json();
                if (res.ok) {
                    localStorage.setItem('token', data.token);
                    window.location.href = './drinks.html';
                } else {
                    alert(data.message || 'Falha na autenticação.');
                }
            } catch (error) {
                alert('Não foi possível conectar ao servidor local. Inicie o backend em http://localhost:3000.');
            }
        };