### 一个完整的interpreter的代码

##### 基于《The Little Schemer》书中讲的原理。代码中有测试用例。支持高阶函数：）

```racket
#lang racket

(define atom?
  (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(define test
  (lambda (name v expected)
    (cond
      ((eq-deep? v expected)
       (display "√. ")
        (display name)
        (display "\n")
        )
      (else
       (display "×. ")
       (display name)
       (display "\n got    [[")
       (display v)
       (display "]]\n expect [[")
       (display expected)
       (display "]]\n"))))
       )
(define eq-deep?
  (lambda (a b)
    (cond
      ((or (atom? a) (atom? b)) (eq? a b))
      ((eq? (length a) (length b))
       (cond
         ((null? a) #t)
         (else (and (eq-deep? (car a) (car b)) (eq-deep? (cdr a) (cdr b)))))
    ))))
(define first
  (lambda (x)
    (car x)))
(define second
  (lambda (x)
    (car (cdr x))))
(define third
  (lambda (x)
    (car (cdr (cdr x)))))
(define find-in-entry
  (lambda (name entry entry-f)
    (cond
      ((null? entry) (entry-f name))
      (else
       (find-in-entry-help name
                           (first entry)
                           (second entry)
                           entry-f))))
  )
(define find-in-entry-help
  (lambda (name names vals entry-f)
    (cond
      ((null? names) (entry-f name))
      ((eq? name (first names)) (first vals))
      (else (find-in-entry-help name
                                (cdr names)
                                (cdr vals)
                                entry-f)))))
(define find-in-table
  (lambda (name table table-f)
    (cond
      ((null? table) (table-f name))
      (else (find-in-entry name
                           (first table)
                           (lambda (name) (find-in-table name (cdr table) table-f))))
      )))
(define ext-table cons)
(define atom-to-action
  (lambda (op)
    (cond
      ((number? op) *const)
      ((eq? #t op) *const)
      ((eq? #f op) *const)
      ((eq? 'cond op) *const)
      ((eq? 'eq? op) *const)
      ((eq? 'add op) *const)
      ((eq? 'sub op) *const)
      ((eq? 'q op) *const)
      ((eq? 'lambda op) *const)
      ((eq? 'car op) *const)
      (else *identifier))))
(define list-to-action
  (lambda (l)
    *application))
(define *application
  ; non-lambda
  (lambda (l env)
    (cond
      ((atom? (car l))
       (cond
         ((eq? (atom-to-action (car l)) *identifier)
          (apply-closure
           (meaning( car l) env)
            (evlis (cdr l) env))
          )
         (else ((atom-to-action (car l)) l env))
         ))
      (else
       (apply-closure (meaning (first l) env) (evlis (cdr l) env))
       ))))
(define evlis
  (lambda (l env)
    (cond
      ((null? l) '())
      (else (cons (meaning (first l) env) (evlis (cdr l) env))))))
(define body-of third)
(define formals-of second)
(define table-of first)
(define build
  (lambda (a b)
    (cons a (cons b '()))))
(define apply-closure
  (lambda (func vals)
    (meaning (body-of func)
             (ext-table (build (formals-of func) vals) (table-of func)))))
(define text-of second)
(define *lambda
  (lambda (l env)
    (cons env (build (second l) (third l)))))
(define else?
  (lambda (l)
    (eq? (first l) 'else)))
(define question-of first)
(define answer-of second)
(define *cond
  (lambda (l env)
    (cond
      ((null? l) (error "reached end of cond clause!"))
      ((else? (first l))
       (meaning (answer-of (first l)) env))
      (else
       (cond
         ((meaning (question-of (first l)) env) (meaning (answer-of (first l)) env))
         (else (*cond (cdr l) env)))))))
(define *const
  (lambda (l env)
    (cond
      ((eq? l #t) #t)
      ((eq? l #f) #f)
      ((number? l) l)
      ((eq? (first l) 'q) (text-of l))
      ((eq? (first l) 'cond) (*cond (cdr l) env))
      ((eq? (first l) 'eq?) (eq-deep? (meaning (second l) env) (meaning (third l) env)))
      ((eq? (first l) 'add) (+ (meaning (second l) env) (meaning (third l) env)))
      ((eq? (first l) 'sub) (- (meaning (second l) env) (meaning (third l) env)))
      ((eq? (first l) 'car) (car (meaning (second l) env)))
      ((eq? (first l) 'lambda) (*lambda l env))
      (else (display "impossible") (error l))
      )))
(define *identifier
  (lambda (l env)
    (cond
      ((atom? l) (find-in-table l env error))
      (else (find-in-table (first l) env error))
      )
    )
  )
(define meaning
  (lambda (prog env)
;    (display "calling : [[ ")
;    (display prog)
;    (display " ]] with env [[ ")
;    (display env)
;    (display " ]]\n")
    (cond
      ((atom? prog) ((atom-to-action prog) prog env))
      (else ((list-to-action prog) prog env)))))
(define value
  (lambda (prog)
    (meaning prog '())))
(test "find-in-table" (find-in-table 'n '(((a b c) (1 2 3)) ((d n)(3 m))) error) 'm)
(test "build-1" (build 1 2) '(1 2))
(test "build-2" (build '(x) 'x) '((x) x))
(test "find-in-entry2" (find-in-entry 'c '((a b c) (1 2 3)) error) 3)
(test "find-in-entry-help" (find-in-entry-help 'b '(a b c) '(1 2 3) error) 2)
(test "test2" '(1 2) '(1 2))
(test "test3" '(2 3 (4 5 (6))) '(2 3 (4 5 (6))))
(test "eq-deep?1" (eq-deep? '(1 2) '(1 2)) #t)
(test "test4" (eq-deep? '(2 3 (4 5 (6))) '(2 3 (4 5 (61)))) #f)
(test "test-atom" (value 12) 12)
(test "test-application-add" (*application '(add 1 2) '()) 3)
(test "test-add" (value '(add 1 2)) 3)
(test "test-lambda" (value '(lambda (x) x)) '(() (x) x))
(test "test-*application" (*application '((lambda (x) x) 1) '()) 1)
(test "test-application" (value '((lambda (x) x) 1)) 1)
(test "test-value-add" (value '((lambda (x y) (add x y)) 1 2)) 3)
(test "test-value-sub" (value '((lambda (x y) (sub x y)) 1 2)) -1)
(test "test-value-eq" (value '((lambda (x y) (eq? x y)) 2 3)) #f)
(test "test-quote" (value '(q (1 2 3))) '(1 2 3))
(test "test-value-quote-eq" (value '((lambda (x y) (eq? x y)) (q (1 2)) (q (1 2)))) #t)
(test "test-car" (value '(car (q (1 2 3)))) 1)
(test "test-cond-simple" (value '(cond (#t (q a)) (else (q b)))) 'a)
(test "test-cond2" (value '(cond
                            ((eq? 1 2) (q a))
                            ((eq? 1 1) (q b))
                            (else (q c)))) 'b)
(test "test-cond3" (value '(cond
                            ((eq? 1 2) (q a))
                            ((eq? 1 2) (q b))
                            (else (q c)))) 'c)
(test "test-apply-closure"
      (apply-closure
       '[ (((a b c)(1 2 3))) (x y) (add x y)]
       '(3 5)
       )
      8)
(test "test-lambda-in-lambda-1"
      (value '((lambda (x) (x 1)) (lambda (y) (add 1 y)))) 2)
(test "test-lambda-in-lambda"
      (value '((lambda (x y) (add (x 10) (y 20))) (lambda (x) (add x 1)) (lambda (y) (add 3 y)))) 34)

```

#### 输出：
```
√. find-in-table
√. build-1
√. build-2
√. find-in-entry2
√. find-in-entry-help
√. test2
√. test3
√. eq-deep?1
√. test4
√. test-atom
√. test-application-add
√. test-add
√. test-lambda
√. test-*application
√. test-application
√. test-value-add
√. test-value-sub
√. test-value-eq
√. test-quote
√. test-value-quote-eq
√. test-car
√. test-cond-simple
√. test-cond2
√. test-cond3
√. test-apply-closure
√. test-lambda-in-lambda-1
√. test-lambda-in-lambda
```
