import socket
import select
import json
from pettingzoo.classic import tictactoe_v3

HOST = "0.0.0.0"
PORT = 8888
RENDER_MODE = "ansi"


def send_json(conn, obj):
    try:
        msg = json.dumps(obj, ensure_ascii=False) + "\n"
        conn.sendall(msg.encode("utf-8"))
    except:
        pass


def recv_available_json(player):
    conn = player["conn"]
    data = conn.recv(4096)
    if not data:
        raise ConnectionError("client disconnected")

    player["buffer"] += data
    messages = []

    while b"\n" in player["buffer"]:
        line, _, player["buffer"] = player["buffer"].partition(b"\n")
        text = line.decode("utf-8").strip()
        if not text:
            continue
        try:
            obj = json.loads(text)
            messages.append(obj)
        except json.JSONDecodeError:
            messages.append({
                "_invalid_json": True,
                "_raw": text
            })

    return messages


def action_to_row_col(action):
    row = action % 3
    col = action // 3
    return row, col


def row_col_to_action(row, col):
    return col * 3 + row


def legal_moves_from_mask(mask):
    moves = []
    for action, v in enumerate(mask):
        if int(v) == 1:
            row, col = action_to_row_col(action)
            moves.append([row, col])
    return moves


def board_chars_from_obs(obs):
    board = obs["observation"]
    grid = []
    for r in range(3):
        row_chars = []
        for c in range(3):
            cell = board[r, c]
            if cell[0] == 1:
                ch = "X"
            elif cell[1] == 1:
                ch = "O"
            else:
                ch = " "
            row_chars.append(ch)
        grid.append(row_chars)
    return grid


def render_ascii_from_obs(obs):
    grid = board_chars_from_obs(obs)
    lines = []
    lines.append("    0   1   2")
    lines.append("  +---+---+---+")
    for r in range(3):
        lines.append(f"{r} | {grid[r][0]} | {grid[r][1]} | {grid[r][2]} |")
        lines.append("  +---+---+---+")
    return "\n".join(lines)


def print_board(obs):
    print(render_ascii_from_obs(obs))


def send_state(conn, obs, message, legal_moves=None):
    payload = {
        "type": "state",
        "message": message,
        "board": board_chars_from_obs(obs),
    }
    if legal_moves is not None:
        payload["legal_moves"] = legal_moves
    send_json(conn, payload)


def send_turn_prompt(player, obs, legal_moves):
    send_json(player["conn"], {
        "type": "your_turn",
        "message": "Your turn. Enter move as JSON: {\"type\":\"move\",\"row\":0,\"col\":0}",
        "board": board_chars_from_obs(obs),
        "legal_moves": legal_moves
    })


def parse_move_json(obj):
    if not isinstance(obj, dict):
        return None, "Message must be a JSON object"

    if obj.get("type") != "move":
        return None, "JSON field 'type' must be 'move'"

    if "row" not in obj or "col" not in obj:
        return None, "JSON move must contain integer fields 'row' and 'col'"

    row = obj.get("row")
    col = obj.get("col")

    if not isinstance(row, int) or not isinstance(col, int):
        return None, "Fields 'row' and 'col' must both be integers"

    if not (0 <= row <= 2 and 0 <= col <= 2):
        return None, "Fields 'row' and 'col' must both be between 0 and 2"

    return (row, col), None


def main():
    env = tictactoe_v3.env(render_mode=RENDER_MODE)
    env.reset()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(2)

    print(f"Server listening on {HOST}:{PORT}")
    print("Waiting for 2 players...")

    print("Waiting for Player 1...")
    conn1, addr1 = server.accept()
    print("Player 1 connected from", addr1)
    send_json(conn1, {
        "type": "welcome",
        "you": "Player 1",
        "symbol": "X",
        "message": "Welcome! You are Player 1 (X)."
    })

    print("Waiting for Player 2...")
    conn2, addr2 = server.accept()
    print("Player 2 connected from", addr2)
    send_json(conn2, {
        "type": "welcome",
        "you": "Player 2",
        "symbol": "O",
        "message": "Welcome! You are Player 2 (O)."
    })

    players = {
        "player_1": {
            "conn": conn1,
            "name": "Player 1",
            "symbol": "X",
            "buffer": b"",
        },
        "player_2": {
            "conn": conn2,
            "name": "Player 2",
            "symbol": "O",
            "buffer": b"",
        },
    }

    final_rewards = None

    try:
        obs, _, _, _, _ = env.last()
        print_board(obs)

        first_agent = env.agent_selection
        for agent in env.possible_agents:
            legal_moves = legal_moves_from_mask(obs["action_mask"]) if agent == first_agent else []
            send_state(
                players[agent]["conn"],
                obs,
                "Game start",
                legal_moves
            )

        while env.agents:
            agent = env.agent_selection
            obs, reward, termination, truncation, info = env.last()

            current = players[agent]
            other_agent = [a for a in env.possible_agents if a != agent][0]
            other = players[other_agent]

            if termination or truncation:
                if final_rewards is None:
                    final_rewards = dict(env.rewards)
                env.step(None)
                continue

            mask = obs["action_mask"]
            legal_moves = legal_moves_from_mask(mask)

            send_turn_prompt(current, obs, legal_moves)
            send_json(other["conn"], {
                "type": "info",
                "message": f"Waiting for {current['name']} ({current['symbol']}) to move..."
            })

            move_done = False

            while not move_done:
                readable, _, _ = select.select([current["conn"], other["conn"]], [], [])

                for conn in readable:
                    owner_agent = agent if conn is current["conn"] else other_agent
                    owner = players[owner_agent]

                    try:
                        messages = recv_available_json(owner)
                    except Exception:
                        if owner_agent == agent:
                            send_json(other["conn"], {
                                "type": "info",
                                "message": "Other player disconnected. Game over."
                            })
                        else:
                            send_json(current["conn"], {
                                "type": "info",
                                "message": "Other player disconnected. Game over."
                            })
                        print(f"{owner['name']} disconnected.")
                        return

                    for msg in messages:
                        if owner_agent != agent:
                            send_json(owner["conn"], {
                                "type": "error",
                                "message": "It is not your turn. Please wait."
                            })
                            continue

                        if isinstance(msg, dict) and msg.get("_invalid_json"):
                            send_json(owner["conn"], {
                                "type": "error",
                                "message": "Invalid JSON. Please send one JSON object per line like {\"type\":\"move\",\"row\":0,\"col\":0}"
                            })
                            continue

                        parsed, err = parse_move_json(msg)
                        if err is not None:
                            send_json(owner["conn"], {
                                "type": "error",
                                "message": err
                            })
                            continue

                        row, col = parsed
                        action = row_col_to_action(row, col)

                        if int(mask[action]) != 1:
                            send_json(owner["conn"], {
                                "type": "error",
                                "message": "Illegal move. That square is not available."
                            })
                            continue

                        print(f"{owner['name']} ({owner['symbol']}) played ({row}, {col})")
                        env.step(action)

                        send_json(owner["conn"], {
                            "type": "info",
                            "message": f"You played ({row}, {col})"
                        })

                        send_json(other["conn"], {
                            "type": "info",
                            "message": f"{owner['name']} ({owner['symbol']}) played ({row}, {col})"
                        })

                        if env.agents:
                            next_obs, _, _, _, _ = env.last()
                            print_board(next_obs)

                            next_agent = env.agent_selection
                            next_mask = next_obs["action_mask"]
                            next_legal_moves = legal_moves_from_mask(next_mask)

                            for a in env.possible_agents:
                                legal = next_legal_moves if a == next_agent else []
                                send_state(
                                    players[a]["conn"],
                                    next_obs,
                                    "Board updated",
                                    legal
                                )

                        if any(env.terminations.values()) or any(env.truncations.values()):
                            final_rewards = dict(env.rewards)

                        move_done = True
                        break

                    if move_done:
                        break

        if final_rewards is None:
            final_rewards = dict(env.rewards)

        print("Game over.", final_rewards)

        final_obs, _, _, _, _ = env.last()
        for a in env.possible_agents:
            send_json(players[a]["conn"], {
                "type": "game_over",
                "message": "Game over",
                "board": board_chars_from_obs(final_obs),
                "rewards": final_rewards
            })

    finally:
        try:
            conn1.close()
        except:
            pass
        try:
            conn2.close()
        except:
            pass
        try:
            server.close()
        except:
            pass
        try:
            env.close()
        except:
            pass


if __name__ == "__main__":
    main()