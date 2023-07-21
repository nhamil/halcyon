echo "=== Running STC Selfplay ==="

cutechess-cli \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 \
    -each proto=uci tc=120/8+0.08 timemargin=9999 book=./varied.bin \
    -pgnout ./selfplay.pgn \
    -bookmode ram \
    -ratinginterval 25 \
    -rounds 64000 \
    -concurrency $1 

echo "Done!" 
