const request = require('supertest');
const app = require('../server');

describe('API Health Check', () => {
    it('Should return 200 and an ok status for /api/health', async () => {
        const response = await request(app).get('/api/health');
        expect(response.status).toBe(200);
        expect(response.body.ok).toBe(true);
        expect(response.body.message).toBe('Servidor online.');
    });
});
