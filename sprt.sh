echo "=== Running STC ==="

cutechess-cli \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 \
    -engine cmd=$3 option.Hash=1024 option.Contempt=0 \
    -each proto=uci tc=120/8+0.08 timemargin=9999 book=./varied.bin \
    -pgnout ./stc.pgn \
    -bookmode ram \
    -sprt elo0=0 elo1=10 alpha=0.01 beta=0.01 \
    -ratinginterval 100 \
    -rounds 2000 \
    -concurrency $1 

echo "=== Running LTC ===" 

cutechess-cli \
    -engine cmd=$2 option.Hash=1024 option.Contempt=0 \
    -engine cmd=$3 option.Hash=1024 option.Contempt=0 \
    -each proto=uci tc=120/60+0.6 timemargin=9999 book=./varied.bin \
    -pgnout ./ltc.pgn \
    -bookmode ram \
    -sprt elo0=0 elo1=10 alpha=0.01 beta=0.01 \
    -ratinginterval 100 \
    -rounds 2000 \
    -concurrency $1 

echo "Done!" 
