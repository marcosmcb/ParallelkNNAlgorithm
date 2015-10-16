principal:
	gcc kNNParaleloOpenMP.c -o kNNParaleloOpenMP -fopenmp
	rm -f kNNParaleloOpenMP.c~

clean:
	rm -f kNNParaleloOpenMP
