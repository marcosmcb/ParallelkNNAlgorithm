principal:
	gcc kNNParaleloMPI.c -o kNNParaleloMPI -fopenmp
	rm -f kNNParaleloOpenMP.c~

clean:
	rm -f kNNParaleloMPI
