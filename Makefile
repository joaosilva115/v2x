CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

v2x: v2x.c
	$(CC) $(CFLAGS) v2x.c -o v2x -lm

# a experiencia: mesmo codigo, dois alcances de radio
demo: v2x
	@echo ">>> alcance grande (300 m): o aviso chega" && ./v2x 300 || true
	@echo && echo ">>> alcance pequeno (50 m): o aviso nao chega" && ./v2x 50 || true

clean:
	rm -f v2x

.PHONY: demo clean