geo_loc_duplicates
==================

Searches for similar locations

####Run script generating points
```bash
python gen_random_data.py 10 10 20 0.5 test2.csv
```

####Compile program
```bash
clang++ -O3 -std=c++11 main.cc -o main -lpsz_csv -lgflags -lm
```

####Requirements
 * https://github.com/orian/trilib
 * https://github.com/orian/psz_utils
 * https://code.google.com/p/gflags
