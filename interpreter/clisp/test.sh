set -x
gcc -fsanitize=address -g lisp.c -o lisp

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

./lisp << 'EOF'
"=== 1. 邏輯與基礎工具 ==="
(define null? (lambda (x) (if x 0 1)))
(define not (lambda (x) (if x 0 1)))
(define true 1)
(define false 0)

"=== 2. 串列存取工具 ==="
(define cadr (lambda (x) (car (cdr x))))
(define cdar (lambda (x) (cdr (car x))))
(define cddr (lambda (x) (cdr (cdr x))))

"=== 3. 數學擴充 ==="
(define add (lambda (a b) (+ a b)))
(define sub (lambda (a b) (- a b)))
(define mul (lambda (a b) (* a b)))
(define square (lambda (x) (* x x)))

"=== 4. 核心串列操作 (List Processing) ==="
(define length 
  (lambda (lst) 
    (if lst 
        (+ 1 (length (cdr lst))) 
        0)))

(define append 
  (lambda (l1 l2) 
    (if l1 
        (cons (car l1) (append (cdr l1) l2)) 
        l2)))

"=== 5. 高階函數 (Higher-Order Functions) ==="
(define map 
  (lambda (f lst) 
    (if lst 
        (cons (f (car lst)) (map f (cdr lst))) 
        (quote ()))))

(define fold 
  (lambda (f acc lst) 
    (if lst 
        (fold f (f acc (car lst)) (cdr lst)) 
        acc)))

"=== 6. 串列反轉 (運用 Helper 函數與遞迴) ==="
(define reverse-aux 
  (lambda (lst acc) 
    (if lst 
        (reverse-aux (cdr lst) (cons (car lst) acc)) 
        acc)))

(define reverse 
  (lambda (lst) 
    (reverse-aux lst (quote ()))))
    
"--- 測試：null? 與 not ---"
(null? (quote ()))
(null? (quote (1 2 3)))
(not 1)

"--- 測試：長度計算 (length) ---"
(define my-list (quote (10 20 30 40)))
(length my-list)

"--- 測試：元素讀取 (cadr) ---"
(cadr my-list)

"--- 測試：串接 (append) 與反轉 (reverse) ---"
(append (quote (1 2)) (quote (3 4)))
(reverse my-list)

"--- 測試：Map (全部平方) ---"
(map square my-list)

"--- 測試：Fold (計算總和) ---"
(fold add 0 my-list)
EOF
