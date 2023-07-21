import sys 
import chess.pgn 

in_file = sys.argv[1] 
out_file = sys.argv[2]
print(f"Loading PGNs from {in_file} and saving FENs to {out_file}") 

games = 0
positions = 0

with open(in_file) as f_in: 
    with open(out_file, 'w') as f_out: 
        while True: 
            game = chess.pgn.read_game(f_in) 
            if game is None: 
                break 

            res = game.headers["Result"] 
            prefix = None

            if res == "1-0": # white wins 
                prefix = "w: " 
            elif res == "0-1": # black wins 
                prefix = "b: "  
            elif res == "1/2-1/2": # draw 
                prefix = "d: "  
            else: # game was not finished 
                continue 

            if prefix is not None: 
                board = game.board() 
                for move in game.mainline(): 
                    board.push(move.move) 
                    # print(move.move, move.comment) 
                    if "M" not in move.comment and "book" not in move.comment: 
                        f_out.write(f"{prefix}{board.fen()}\n")
                        positions += 1

            games += 1 
            print(f"Written {games} games and {positions} positions")
