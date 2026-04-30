from __future__ import annotations

import json
import os
from datetime import datetime, timedelta, timezone
from functools import wraps
from pathlib import Path
from typing import Any
from uuid import uuid4

import jwt
from dotenv import load_dotenv
from flask import Flask, jsonify, redirect, request
from flask_cors import CORS
from werkzeug.security import check_password_hash, generate_password_hash

BACKEND_DIR = Path(__file__).resolve().parent
PYTHON_ROOT = BACKEND_DIR.parent
APP_ROOT = PYTHON_ROOT.parent
DB_PATH = BACKEND_DIR / "data" / "db.json"

load_dotenv(PYTHON_ROOT / ".env")

PORT = int(os.getenv("PORT", "3000"))
JWT_SECRET = os.getenv("JWT_SECRET", "dev_local_secret_change_me")
DEBUG = os.getenv("FLASK_DEBUG", "0") == "1"

app = Flask(__name__, static_folder=str(APP_ROOT), static_url_path="")
CORS(app)


def ensure_db() -> None:
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    if not DB_PATH.exists():
        DB_PATH.write_text(json.dumps({"users": [], "drinks": []}, indent=2), encoding="utf-8")


def read_db() -> dict[str, list[dict[str, Any]]]:
    ensure_db()
    with DB_PATH.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    if not isinstance(data, dict):
        return {"users": [], "drinks": []}

    data.setdefault("users", [])
    data.setdefault("drinks", [])
    return data


def write_db(db: dict[str, list[dict[str, Any]]]) -> None:
    with DB_PATH.open("w", encoding="utf-8") as handle:
        json.dump(db, handle, ensure_ascii=False, indent=2)


def parse_number(value: Any, fallback: float = 0.0) -> float:
    if value in (None, ""):
        return float(fallback)
    try:
        return float(value)
    except (TypeError, ValueError):
        return float(fallback)


def parse_date_key(item: dict[str, Any]) -> datetime:
    raw_value = item.get("date")
    if isinstance(raw_value, str):
        try:
            return datetime.fromisoformat(raw_value.replace("Z", "+00:00"))
        except ValueError:
            pass
    return datetime.min.replace(tzinfo=timezone.utc)


def create_token(user: dict[str, Any]) -> str:
    payload = {
        "id": user["id"],
        "email": user["email"],
        "iat": datetime.now(timezone.utc),
        "exp": datetime.now(timezone.utc) + timedelta(days=7),
    }
    token = jwt.encode(payload, JWT_SECRET, algorithm="HS256")
    return token if isinstance(token, str) else token.decode("utf-8")


def auth_required(view):
    @wraps(view)
    def wrapper(*args, **kwargs):
        auth_header = request.headers.get("Authorization", "")
        token = auth_header[7:] if auth_header.startswith("Bearer ") else auth_header.strip()

        if not token:
            return jsonify({"message": "Acesso negado. Token ausente."}), 401

        try:
            request.user = jwt.decode(token, JWT_SECRET, algorithms=["HS256"])
        except jwt.ExpiredSignatureError:
            return jsonify({"message": "Token inválido ou expirado."}), 401
        except jwt.InvalidTokenError:
            return jsonify({"message": "Token inválido ou expirado."}), 401

        return view(*args, **kwargs)

    return wrapper


@app.route("/")
def index():
    return redirect("/frontend/pages/auth.html")


@app.get("/api/health")
def health():
    return jsonify({"ok": True, "message": "Servidor online."})


@app.post("/api/auth/register")
def register():
    try:
        data = request.get_json(silent=True) or {}
        email = str(data.get("email", "")).strip().lower()
        password = str(data.get("password", ""))

        if not email or not password:
            return jsonify({"message": "E-mail e senha são obrigatórios."}), 400

        if len(password) < 6:
            return jsonify({"message": "A senha deve ter pelo menos 6 caracteres."}), 400

        db = read_db()
        users = db["users"]
        existing = next((user for user in users if str(user.get("email", "")).lower() == email), None)
        if existing:
            return jsonify({"message": "Este e-mail já está cadastrado."}), 409

        user = {
            "id": str(uuid4()),
            "email": email,
            "passwordHash": generate_password_hash(password),
            "createdAt": datetime.now(timezone.utc).isoformat(),
        }

        users.append(user)
        write_db(db)

        return jsonify({"token": create_token(user)}), 201
    except Exception:
        return jsonify({"message": "Erro interno ao cadastrar usuário."}), 500


@app.post("/api/auth/login")
def login():
    try:
        data = request.get_json(silent=True) or {}
        email = str(data.get("email", "")).strip().lower()
        password = str(data.get("password", ""))

        if not email or not password:
            return jsonify({"message": "E-mail e senha são obrigatórios."}), 400

        db = read_db()
        user = next((item for item in db["users"] if str(item.get("email", "")).lower() == email), None)
        if not user:
            return jsonify({"message": "Credenciais inválidas."}), 401

        if not check_password_hash(str(user.get("passwordHash", "")), password):
            return jsonify({"message": "Credenciais inválidas."}), 401

        return jsonify({"token": create_token(user)})
    except Exception:
        return jsonify({"message": "Erro interno ao fazer login."}), 500


@app.get("/api/drinks")
@auth_required
def list_drinks():
    try:
        db = read_db()
        user_id = request.user["id"]
        drinks = [drink for drink in db["drinks"] if drink.get("userId") == user_id]
        drinks.sort(key=parse_date_key, reverse=True)
        return jsonify(drinks)
    except Exception:
        return jsonify({"message": "Erro ao buscar drinks."}), 500


@app.post("/api/drinks")
@auth_required
def create_drink():
    try:
        data = request.get_json(silent=True) or {}
        name = str(data.get("name", "")).strip()
        cost = parse_number(data.get("cost"))

        if not name or cost <= 0:
            return jsonify({"message": "Nome e custo válido são obrigatórios."}), 400

        db = read_db()
        drink = {
            "id": str(uuid4()),
            "userId": request.user["id"],
            "name": name,
            "cost": cost,
            "price": parse_number(data.get("price")),
            "cmv": parse_number(data.get("cmv")),
            "spirits": data.get("spirits") if isinstance(data.get("spirits"), list) else [],
            "ingredients": data.get("ingredients") if isinstance(data.get("ingredients"), list) else [],
            "date": data.get("date") or datetime.now(timezone.utc).isoformat(),
        }

        db["drinks"].append(drink)
        write_db(db)
        return jsonify(drink), 201
    except Exception:
        return jsonify({"message": "Erro ao salvar drink."}), 500


@app.put("/api/drinks/<drink_id>")
@auth_required
def update_drink(drink_id: str):
    try:
        data = request.get_json(silent=True) or {}
        db = read_db()
        drinks = db["drinks"]
        index = next(
            (position for position, drink in enumerate(drinks)
             if drink.get("id") == drink_id and drink.get("userId") == request.user["id"]),
            None,
        )

        if index is None:
            return jsonify({"message": "Drink não encontrado."}), 404

        current = drinks[index]
        updated = {
            **current,
            "name": str(data.get("name", current.get("name", ""))).strip() or current.get("name", ""),
            "cost": parse_number(data.get("cost"), current.get("cost", 0)),
            "price": parse_number(data.get("price"), current.get("price", 0)),
            "cmv": parse_number(data.get("cmv"), current.get("cmv", 0)),
            "spirits": data.get("spirits") if isinstance(data.get("spirits"), list) else current.get("spirits", []),
            "ingredients": data.get("ingredients") if isinstance(data.get("ingredients"), list) else current.get("ingredients", []),
            "date": data.get("date") or current.get("date"),
        }

        drinks[index] = updated
        write_db(db)
        return jsonify(updated)
    except Exception:
        return jsonify({"message": "Erro ao atualizar drink."}), 500


@app.delete("/api/drinks/<drink_id>")
@auth_required
def delete_drink(drink_id: str):
    try:
        db = read_db()
        drinks = db["drinks"]
        index = next(
            (position for position, drink in enumerate(drinks)
             if drink.get("id") == drink_id and drink.get("userId") == request.user["id"]),
            None,
        )

        if index is None:
            return jsonify({"message": "Drink não encontrado."}), 404

        drinks.pop(index)
        write_db(db)
        return jsonify({"message": "Drink removido com sucesso."})
    except Exception:
        return jsonify({"message": "Erro ao excluir drink."}), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=PORT, debug=DEBUG)
