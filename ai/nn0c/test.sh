set -x

gcc -shared -fPIC -O3 -o libnn0.so cnn0.c nn0.c -lm

gcc -O3 -o examples/test_cnn0 examples/test_cnn0.c cnn0.c nn0.c -lm -lz

gcc gpt0.c nn0.c examples/test_gpt0.c -o examples/test_gpt0
./examples/test_gpt0 ../_data/corpus/tw.txt

# python test_nn0.py
