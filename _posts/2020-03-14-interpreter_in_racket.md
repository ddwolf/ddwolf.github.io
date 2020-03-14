### 一个完整的interpreter的代码

##### 基于《The Little Schemer》书中讲的原理。代码中有测试用例

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
      ((atom? a) (eq? a b))
      ((eq? (length a) (length b))
       (cond
         ((null? a) #t)
         (else (and (eq-deep? (car a) (car b)) (eq-deep? (cdr a) (cdr b)))))
    ))))
(test "test2" '(1 2) '(1 2))
(test "test3" '(2 3 (4 5 (6))) '(2 3 (4 5 (6))))
(test "test1" 1 2)
(test "eq-deep?1" (eq-deep? '(1 2) '(1 2)) #t)
(test "test4" (eq-deep? '(2 3 (4 5 (6))) '(2 3 (4 5 (61)))) #f)
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
(test "find-in-entry2" (find-in-entry 'c '((a b c) (1 2 3)) error) 3)
(test "find-in-entry-help" (find-in-entry-help 'b '(a b c) '(1 2 3) error) 2)
(define find-in-table
  (lambda (name table table-f)
    (cond
      ((null? table) (table-f name))
      (else (find-in-entry name 
                           (first table)
                           (lambda (name) (find-in-table name (cdr table) table-f))))
      )))
(test "find-in-table" (find-in-table 'n '(((a b c) (1 2 3)) ((d n)(3 m))) error) 'm)
(define ext-table cons)
(define atom-to-action
  (lambda (op)
    (cond
      ((number? op) *const)
      ((eq? #t op) *const)
      ((eq? #f op) *const)
      ((eq? 'q op) *const)
      ((eq? 'lambda op) *const)
      ((eq? 'cond op) *const)
      ((eq? 'eq? op) *const)
      ((eq? 'add op) *const)
      ((eq? 'sub op) *const)
      (else *identifier))))
(define list-to-action
  (lambda (l)
    (cond
      ((null? l) (display "eval emtpy list?"))
      (else *application)))
  )
(define *application
  ; non-lambda
  (lambda (l env)
    (apply-closure (meaning (first l) env) (evlis (cdr l) env))
    ))
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
(test "build-1" (build 1 2) '(1 2))
(test "build-2" (build '(x) 'x) '((x) x))
(define apply-closure
  (lambda (func vals)
    (meaning (body-of func)
             (ext-table (build (formals-of func)
                               vals) (table-of func)))))
(define text-of second)
(define *lambda
  (lambda (l env)
    (cons env (build (second l) (third l)))))
(define else?
  (lambda (l)
    (eq? (first l) 'else)))
(define answer-of second)
(define question-of third)
(define *cond
  (lambda (l env)
    (cond
      ((null? l) (error "reached end of cond clause!"))
      ((else? (first (first l)))
       (meaning (answer-of l ) env))
      (else
      (cond
        ((meaning (question-of l) env) (meaning (answer-of l) env))
        (else (*cond (cdr l) env)))))))
(define *const
  (lambda (l env)
    (cond
      ((eq? l #t) #t)
      ((eq? l #f) #f)
      ((number? l) l)
      ((eq? (first l) 'q) (text-of l))
      ((eq? (first l) 'lambda) (*lambda l env))
      ((eq? (first l) 'cond) (*cond (cdr l) env))
      ((eq? (first l) 'eq?) (eq? (meaning (second l) env) (meaning (third l) env)))
      ((eq? (first l) 'add) (+ (meaning (second l) env) (meaning (third l) env)))
      ((eq? (first l) 'sub) (- (meaning (second l) env) (meaning (third l) env)))
      (else (display "impossible") (error l))
      )))
(define *identifier
  (lambda (name env)
    (find-in-table name env error)))
(define meaning
  (lambda (prog env)
    (cond
      ((null? prog) '())
      ((atom? prog) ((atom-to-action prog) prog env))
      ((atom? (first prog)) ((atom-to-action (first prog)) prog env))
      (else ((list-to-action prog) prog env)))))
(define value
  (lambda (prog)
    (meaning prog '())))
(test "test-add" (value '(add 1 2)) 3)
(test "test-lambda" (value '(lambda (x) x)) '(() (x) x))
(test "test-*application" (*application '((lambda (x) x) 1) '()) 1)
(test "test-application" (value '((lambda (x) x) 1)) 1)
```
