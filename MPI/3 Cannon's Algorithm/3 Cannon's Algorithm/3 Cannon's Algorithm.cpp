// 3 Cannon's Algorithm.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include <iostream>
#include <random>
#include <iomanip>
#include <string>
#include "mpi.h"

using namespace std;

inline double ScalarProduct(double *a, double *b, int size)
{
	double res = 0.0;

	for (int i = 0; i < size; i++)
	{
		res += a[i] * b[i];
	}

	return res;
}

void InitMatrix(double *matrix, int rows, int cols)
{
	std::default_random_engine generator(rand());
	std::uniform_real_distribution<double> distribution(-10.0, 10.0);

	for (int i = 0; i < rows * cols; i++)
	{
		matrix[i] = distribution(generator);
	}
}

void PrintMatrix(double *matrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			cout << setw(7) << matrix[i * cols + j] << " ";
		}
		cout << endl;
	}
}


int main(int argc, char *argv[])
{
	srand((unsigned int)time(nullptr));

	const int ROOT = 0;
	double time;

	int procNum;
	int procRank;

	int rowsA = (argc == 1) ? 6 : stoi(argv[1]);
	int colsA_rowsB = (argc == 1) ? 7 : stoi(argv[2]);
	int colsB = (argc == 1) ? 8 : stoi(argv[3]);

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &procNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	double *matrixA = nullptr;
	double *matrixB = nullptr;
	double *matrixC = nullptr;

	if (procRank == ROOT)
	{
		matrixA = new double[rowsA * colsA_rowsB];
		matrixB = new double[colsA_rowsB * colsB];
		matrixC = new double[rowsA * colsB];

		InitMatrix(matrixA, rowsA, colsA_rowsB);
		InitMatrix(matrixB, colsA_rowsB, colsB);

		bool showMatrix = false;
		if (argc == 5 && string(argv[4]) == "true")
		{
			showMatrix = true;
		}

		if (showMatrix)
		{
			cout << "Matrix A with size " << rowsA << "x" << colsA_rowsB << endl;
			PrintMatrix(matrixA, rowsA, colsA_rowsB);
			cout << endl;

			cout << "Matrix B with size " << colsA_rowsB << "x" << colsB << endl;
			PrintMatrix(matrixB, colsA_rowsB, colsB);
			cout << endl;
		}
	}

	time = MPI_Wtime();


	return 0;
}