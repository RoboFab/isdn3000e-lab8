import socket
import select
from pettingzoo.classic import tictactoe_v3

HOST = "0.0.0.0"
PORT = 9999
RENDER_MODE = "ansi" 


def send_line(conn, text):
    try:
        conn.sendall((text + "\n").encode("utf-8"))
    except:
        pass


def recv_available_lines(player):
    conn = player["conn"]
    data = conn.recv(4096)
    if not data:
        raise ConnectionError("client disconnected")
    player["buffer"] += data

    lines = []
    while b"\n" in player["buffer"]:
        line, _, player["buffer"] = player["buffer"].partition(b"\n")
        lines.append(line.decode("utf-8").strip())
    return lines


def action_to_row_col(action):
    # PettingZoo tictactoe action mapping:
    # 0 | 3 | 6
    # 1 | 4 | 7
    # 2 | 5 | 8
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
            moves.append((row, col))
    return moves


def render_ascii_from_obs(obs):
    board = obs["observation"]
    lines = []
    lines.append("    0   1   2")
    lines.append("  +---+---+---+")
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
        lines.append(f"{r} | {row_chars[0]} | {row_chars[1]} | {row_chars[2]} |")
        lines.append("  +---+---+---+")
    return "\n".join(lines)


def print_board(env, obs):
    if RENDER_MODE == "human":
        try:
            env.render()
        except:
            pass
    else:
        print(render_ascii_from_obs(obs))


def prompt_turn(current, legal_moves):
    legal_text = " ".join([f"({r},{c})" for r, c in legal_moves])
    send_line(current["conn"], f"YOUR_TURN {current['name']} {current['symbol']}")
    send_line(current["conn"], f"LEGAL_MOVES {legal_text}")
    send_line(current["conn"], "INPUT Please enter: row col")


def parse_move_line(line):
    parts = line.split()
    if len(parts) != 2:
        return None, "ERROR Bad format. Please enter exactly: row col"

    try:
        row = int(parts[0])
        col = int(parts[1])
    except ValueError:
        return None, "ERROR row and col must be integers"

    if not (0 <= row <= 2 and 0 <= col <= 2):
        return None, "ERROR row and col must both be between 0 and 2"

    return (row, col), None


def main():
    env = tictactoe_v3.env(render_mode=RENDER_MODE)
    env.reset()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(2)

    print(f"Server listening on {HOST}:{PORT}")
    print("Open two clients to join the game.")

    print("Waiting for Player 1...")
    conn1, addr1 = server.accept()
    print("Player 1 connected from", addr1)
    send_line(conn1, "WELCOME You are Player 1 (X)")

    print("Waiting for Player 2...")
    conn2, addr2 = server.accept()
    print("Player 2 connected from", addr2)
    send_line(conn2, "WELCOME You are Player 2 (O)")

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
        print_board(env, obs)

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

            prompt_turn(current, legal_moves)
            send_line(other["conn"], f"INFO Waiting for {current['name']} ({current['symbol']}) to move...")

            move_done = False

            while not move_done:
                readable, _, _ = select.select([current["conn"], other["conn"]], [], [])

                for conn in readable:
                    owner_agent = agent if conn is current["conn"] else other_agent
                    owner = players[owner_agent]

                    try:
                        lines = recv_available_lines(owner)
                    except Exception:
                        if owner_agent == agent:
                            send_line(other["conn"], "INFO Other player disconnected. Game over.")
                            print(f"{owner['name']} disconnected.")
                        else:
                            send_line(current["conn"], "INFO Other player disconnected. Game over.")
                            print(f"{owner['name']} disconnected.")
                        return

                    for line in lines:
                        if owner_agent != agent:
                            send_line(owner["conn"], "NOT_YOUR_TURN It is not your turn. Please wait.")
                            continue

                        if not line:
                            send_line(owner["conn"], "ERROR Empty input. Please enter: row col")
                            continue

                        parsed, err = parse_move_line(line)
                        if err is not None:
                            send_line(owner["conn"], err)
                            send_line(owner["conn"], "INPUT Please enter again: row col")
                            continue

                        row, col = parsed
                        action = row_col_to_action(row, col)

                        if int(mask[action]) != 1:
                            send_line(owner["conn"], "ERROR Illegal move. That square is not available.")
                            send_line(owner["conn"], "INPUT Please enter again: row col")
                            continue

                        print(f"{owner['name']} ({owner['symbol']}) played ({row}, {col})")
                        env.step(action)

                        next_obs = None
                        if env.agents:
                            next_obs, _, _, _, _ = env.last()
                            print_board(env, next_obs)

                        send_line(owner["conn"], f"OK You played ({row}, {col})")
                        send_line(other["conn"], f"INFO {owner['name']} ({owner['symbol']}) played ({row}, {col})")

                        if any(env.terminations.values()) or any(env.truncations.values()):
                            final_rewards = dict(env.rewards)

                        move_done = True
                        break

                    if move_done:
                        break

        if final_rewards is None:
            final_rewards = dict(env.rewards)

        print("Game over.", final_rewards)
        for a in env.possible_agents:
            send_line(players[a]["conn"], f"GAME_OVER rewards={final_rewards}")

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