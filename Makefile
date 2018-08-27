nashell: *.c
	gcc -g -o $@ $^

.PHONY: clean
clean:
	-rm nashell
