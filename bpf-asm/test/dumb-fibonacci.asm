Do 1 10 1 1	;x=1
Do 4 10 1 1	;y=1
Echo 1 		;print x
Do 5 10 1 15 	;timeout=15
PtrTo 6 	;store location to myrepeat
Do 5 30 1 1 	;timeout-1
Do 1 20 2 4 	;x=x+y
Do 2 10 2 1 	;ox=x
Do 3 10 2 4 	;oy=y
Do 4 10 2 2 	;y=ox
Do 1 10 2 3 	;x=oy
Echo 1 		;print x
BoolDie 5 	;exit if timeout<0
PtrFrom 6 	;goto myrepeat