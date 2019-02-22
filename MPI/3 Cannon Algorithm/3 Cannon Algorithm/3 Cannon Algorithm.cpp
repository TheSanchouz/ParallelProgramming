// 3 Cannon Algorithm.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include <iostream>
#include <random>
#include <iomanip>
#include <string>
#include "mpi.h"
#include <chrono>
#include <thread>

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
	std::uniform_int_distribution<> distribution(-10, 10);

	for (int i = 0; i < rows * cols; i++)
	{
		matrix[i] = distribution(generator);
	}
}
void TransposeMatrix(double *matrix, double *transMatrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			transMatrix[j * rows + i] = matrix[i * cols + j];
		}
	}
}
void PrintMatrix(double *matrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			std::cout << setw(7) << matrix[i * cols + j] << " ";
		}
		std::cout << endl;
	}
}


int main(int argc, char *argv[])
{
	srand((unsigned int)time(nullptr));

	const int ROOT = 0;
	const int DATA_TAG_SEND_MATRIX_A_BLOCKS = 1;
	const int DATA_TAG_SEND_MATRIX_B_BLOCKS = 2;
	double time;

	int procNum;
	int procRank;


	int rowsA = (argc == 1) ? 5 : stoi(argv[1]);
	int colsA_rowsB = (argc == 1) ? 6 : stoi(argv[2]);
	int colsB = (argc == 1) ? 7 : stoi(argv[3]);



	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &procNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);


	double *matrixA = nullptr;
	double *matrixB = nullptr;
	double *matrixC = nullptr;

	if (procRank == ROOT)
	{
		//выделение памяти под матрицы
		matrixA = new double[rowsA * colsA_rowsB];
		matrixB = new double[colsA_rowsB * colsB];
		matrixC = new double[rowsA * colsB];

		//заполнение матриц случайными числами
		InitMatrix(matrixA, rowsA, colsA_rowsB);
		InitMatrix(matrixB, colsA_rowsB, colsB);


		bool showMatrix = false;
		//if (argc == 5 && string(argv[4]) == "true")
		{
			showMatrix = true;
		}

		//вывод матриц на экран
		if (showMatrix)
		{
			cout << "Matrix A with size " << rowsA << "x" << colsA_rowsB << endl;
			PrintMatrix(matrixA, rowsA, colsA_rowsB);
			cout << endl;

			cout << "Matrix B with size " << colsA_rowsB << "x" << colsB << endl;
			PrintMatrix(matrixB, colsA_rowsB, colsB);
			cout << endl;

			/*cout << "Transpose Matrix B with size " << colsB << "x" << colsA_rowsB << endl;
			PrintMatrix(transMatrixB, colsB, colsA_rowsB);
			cout << endl;*/
		}
	}


	MPI_Comm MPI_MY_COMM_CART;
	int dims[2] = { 0, 0 };
	const int periods[2] = { 1, 1 };

	//создание декартовой решетки
	MPI_Dims_create(procNum, 2, dims);
	MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &MPI_MY_COMM_CART);

	int myCoords[2];
	MPI_Cart_coords(MPI_MY_COMM_CART, procRank, 2, myCoords);


	int myCountOfRowsA = rowsA / dims[0] + (myCoords[0] < (rowsA % dims[0]) ? 1 : 0);
	int myCountOfColsA = colsA_rowsB / dims[1] + (myCoords[1] < (colsA_rowsB % dims[1]) ? 1 : 0);

	//int myCountOfRowsB = colsA_rowsB / dims[1] + (myCoords[1] < (colsA_rowsB % dims[1]) ? 1 : 0);
	//int myCountOfColsB = colsB / dims[0] + (myCoords[0] < (colsB % dims[0]) ? 1 : 0);

	int myCountOfRowsB = colsA_rowsB / dims[0] + (myCoords[0] < (colsA_rowsB % dims[0]) ? 1 : 0);
	int myCountOfColsB = colsB / dims[1] + (myCoords[1] < (colsB % dims[1]) ? 1 : 0);

	double *bufA = new double[myCountOfRowsA * myCountOfColsA];
	double *bufB = new double[myCountOfRowsB * myCountOfColsB];

	MPI_Request *requestA = (procRank == 0) ? new MPI_Request[procNum] : nullptr;
	MPI_Request *requestB = (procRank == 0) ? new MPI_Request[procNum] : nullptr;

	if (procRank == ROOT)
	{
		int sizesA[2] = { rowsA, colsA_rowsB };
		//int sizesB[2] = { colsB, colsA_rowsB };
		int sizesB[2] = { colsA_rowsB, colsB };

		int everyHasRowsA = rowsA / dims[0];
		int everyHasColsA = colsA_rowsB / dims[1];

		//int everyHasRowsB = colsA_rowsB / dims[1];
		//int everyHasColsB = colsB/ dims[0];

		int everyHasRowsB = colsA_rowsB / dims[0];
		int everyHasColsB = colsB / dims[1];

		MPI_Datatype MPI_MY_DATATYPE_MATRIX_A_BLOCK;
		MPI_Datatype MPI_MY_DATATYPE_MATRIX_B_BLOCK;

		for (int i = ROOT; i < procNum ; i++)
		{
			int coords[2];
			MPI_Cart_coords(MPI_MY_COMM_CART, i, 2, coords);

			int addictiveRowsA = (coords[0] < (rowsA % dims[0]) ? 1 : 0);
			int addictiveColsA = (coords[1] < (colsA_rowsB % dims[1]) ? 1 : 0);
			//int addictiveRowsB = (coords[1] < (colsA_rowsB % dims[1]) ? 1 : 0);
			//int addictiveColsB = (coords[0] < (colsB % dims[0]) ? 1 : 0);
			int addictiveRowsB = (coords[0] < (colsA_rowsB % dims[0]) ? 1 : 0);
			int addictiveColsB = (coords[1] < (colsB % dims[1]) ? 1 : 0);

			int sizeOfSendRowsA = everyHasRowsA + addictiveRowsA;
			int sizeOfSendColsA = everyHasColsA + addictiveColsA;
			int sizeOfSendRowsB = everyHasRowsB + addictiveRowsB;
			int sizeOfSendColsB = everyHasColsB + addictiveColsB;

			int subsizesA[2] = { sizeOfSendRowsA, sizeOfSendColsA };
			int subsizesB[2] = { sizeOfSendRowsB, sizeOfSendColsB };

			int startsARows = rowsA - (dims[0] - coords[0]) * everyHasRowsA - 
				((rowsA % dims[0] - coords[0] > 0) ? rowsA % dims[0] - coords[0] : 0);

			int startsACols = colsA_rowsB - (dims[1] - coords[1]) * everyHasColsA -
				((colsA_rowsB % dims[1] - coords[1] > 0) ? colsA_rowsB % dims[1] - coords[1] : 0);

			//int startsBRows = colsA_rowsB - (dims[1] - coords[1]) * everyHasRowsB -
			//	((colsA_rowsB % dims[1] - coords[1] > 0) ? colsA_rowsB % dims[1] - coords[1] : 0);

			//int startsBCols = colsB - (dims[0] - coords[0]) * everyHasColsB -
			//	((colsB % dims[0] - coords[0] > 0) ? colsB % dims[0] - coords[0] : 0);

			int startsBRows = colsA_rowsB - (dims[0] - coords[0]) * everyHasRowsB -
				((colsA_rowsB % dims[0] - coords[0] > 0) ? colsA_rowsB % dims[0] - coords[0] : 0);

			int startsBCols = colsB - (dims[1] - coords[1]) * everyHasColsB -
				((colsB % dims[1] - coords[1] > 0) ? colsB % dims[1] - coords[1] : 0);


			//std::cout << "process " << procRank << " with coord (" << coords[0] << ", " << coords[1] << ")"
			//	<< " has a submatrix A start x = " << startsARows << endl;
			//std::cout << "process " << procRank << " with coord (" << coords[0] << ", " << coords[1] << ")"
			//	<< " has a submatrix A start y = " << startsACols << endl;
			//std::cout << "process " << procRank << " with coord (" << coords[0] << ", " << coords[1] << ")"
			//	<< " has a submatrix B start x = " << startsBRows << endl;
			//std::cout << "process " << procRank << " with coord (" << coords[0] << ", " << coords[1] << ")"
			//	<< " has a submatrix B start y = " << startsBCols << endl;

			int startsA[2] = { startsARows, startsACols };
			int startsB[2] = { startsBRows, startsBCols };

			MPI_Type_create_subarray(2, sizesA, subsizesA, startsA, MPI_ORDER_C, MPI_DOUBLE, &MPI_MY_DATATYPE_MATRIX_A_BLOCK);
			MPI_Type_create_subarray(2, sizesB, subsizesB, startsB, MPI_ORDER_C, MPI_DOUBLE, &MPI_MY_DATATYPE_MATRIX_B_BLOCK);

			MPI_Type_commit(&MPI_MY_DATATYPE_MATRIX_A_BLOCK);
			MPI_Type_commit(&MPI_MY_DATATYPE_MATRIX_B_BLOCK);

			int nextProcA;
			int nextProcB;
			MPI_Cart_rank(MPI_MY_COMM_CART, new int[2]{ coords[0], coords[1] - coords[0] }, &nextProcA);
			MPI_Cart_rank(MPI_MY_COMM_CART, new int[2]{ coords[0] - coords[1], coords[1] }, &nextProcB);

			MPI_Isend(matrixA, 1, MPI_MY_DATATYPE_MATRIX_A_BLOCK, nextProcA, DATA_TAG_SEND_MATRIX_A_BLOCKS, MPI_COMM_WORLD, &requestA[i]);
			MPI_Isend(matrixB, 1, MPI_MY_DATATYPE_MATRIX_B_BLOCK, nextProcB, DATA_TAG_SEND_MATRIX_B_BLOCKS, MPI_COMM_WORLD, &requestB[i]);

			MPI_Type_free(&MPI_MY_DATATYPE_MATRIX_A_BLOCK);
			MPI_Type_free(&MPI_MY_DATATYPE_MATRIX_B_BLOCK);
		}
	}
	
	//прием блоков матрицы A и B
	MPI_Status statusA;
	MPI_Status statusB;
	MPI_Recv(bufA, myCountOfRowsA * myCountOfColsA, MPI_DOUBLE, ROOT, DATA_TAG_SEND_MATRIX_A_BLOCKS, MPI_COMM_WORLD, &statusA);
	MPI_Recv(bufB, myCountOfRowsB * myCountOfColsB, MPI_DOUBLE, ROOT, DATA_TAG_SEND_MATRIX_B_BLOCKS, MPI_COMM_WORLD, &statusB);

	delete[] requestA;
	delete[] requestB;

	
	double *bufC = new double[myCountOfRowsA * myCountOfColsB]{ 0 };

	for (int count = 0; count < max(dims[0], dims[1]); count++)
	{
		double *transBufB = new double[myCountOfRowsA * myCountOfColsB];
		TransposeMatrix(bufB, transBufB, myCountOfRowsA, myCountOfColsB);

		for (int i = 0; i < myCountOfRowsA; i++)
		{
			for (int j = 0; j < myCountOfColsB; j++)
			{
				bufC[i * myCountOfRowsA + j] += ScalarProduct(bufA + i * myCountOfRowsB, transBufB + j * myCountOfRowsB, myCountOfRowsB);
			}
		}

		//this_thread::sleep_for(chrono::milliseconds(400 * procRank));

		//std::cout << "process " << procRank << " with coord (" << myCoords[0] << ", " << myCoords[1] << ")"
		//	<< " has a submatrix A with size " << myCountOfRowsA << "x" << myCountOfColsA << endl;
		//PrintMatrix(bufA, myCountOfRowsA, myCountOfColsA);
		//std::cout << "process " << procRank << " with coord (" << myCoords[0] << ", " << myCoords[1] << ")"
		//	<< " has a submatrix B with size " << myCountOfRowsB << "x" << myCountOfColsB << endl;
		//PrintMatrix(bufB, myCountOfRowsB, myCountOfColsB);
		//std::cout << endl;
		//std::cout << "process " << procRank << " with coord (" << myCoords[0] << ", " << myCoords[1] << ")"
		//	<< " has a submatrix C with size " << myCountOfRowsA << "x" << myCountOfColsA << endl;
		//PrintMatrix(bufC, myCountOfRowsA, myCountOfColsA);
		//std::cout << endl;

		if (dims[0] != 1)
		{
			int prevRank;
			int nextRank;

			MPI_Cart_shift(MPI_MY_COMM_CART, 1, -1, &prevRank, &nextRank);
			MPI_Status status;
			MPI_Sendrecv_replace(bufA, myCountOfRowsA * myCountOfColsB, MPI_DOUBLE, nextRank, 3, prevRank, 3, MPI_COMM_WORLD, &status);

		}
		if (dims[1] != 1)
		{
			int prevRank;
			int nextRank;

			MPI_Cart_shift(MPI_MY_COMM_CART, 0, -1, &prevRank, &nextRank);
			MPI_Status status;
			MPI_Sendrecv_replace(bufB, myCountOfRowsA * myCountOfColsB, MPI_DOUBLE, nextRank, 4, prevRank, 4, MPI_COMM_WORLD, &status);
		}
	}


	this_thread::sleep_for(chrono::milliseconds(400 * procRank));

	std::cout << "process " << procRank << " with coord (" << myCoords[0] << ", " << myCoords[1] << ")"
		<< " has a submatrix C with size " << myCountOfRowsA << "x" << myCountOfColsA << endl;
	PrintMatrix(bufC, myCountOfRowsA, myCountOfColsA);
	std::cout << endl;

	MPI_Finalize();

	return 0;
}