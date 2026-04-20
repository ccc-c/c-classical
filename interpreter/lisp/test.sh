set -x
gcc lisp.c -o lisp
./lisp < "(cdr (quote (10 20 30)))"

./lisp << 'EOF'
(car (quote (1 2 3)))
(cdr (quote (1 2 3)))
(cons 1 (quote (2 3)))
(define second (lambda (lst) (car (cdr lst))))
(second (quote (100 200 300)))
"Hello, World!"
(define msg "This is a string")
msg
(cons "First" (quote ("Second" "Third")))
EOF
