#include "pch.h"
#include <iostream>
#include <random>
#include <iomanip>
#include <string>
#include "mpi.h"

inline int scalarProduct(int *a, int *b, int size)
{
	int res = 0;

	for (int i = 0; i < size; i++)
	{
		res += a[i] * b[i];
	}

	return res;
}
void initMatrix(int *matrix, int rows, int cols)
{
	std::default_random_engine generator(rand());
	std::uniform_int_distribution<> distribution(-10, 10);

	for (int i = 0; i < rows * cols; i++)
	{
		matrix[i] = distribution(generator);
	}
}
void transposeMatrix(int *matrix, int *transMatrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			transMatrix[j * rows + i] = matrix[i * cols + j];
		}
	}
}
void printMatrix(int *matrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			std::cout << std::setw(5) << matrix[i * cols + j] << " ";
		}

		std::cout << std::endl;
	}
}

int main(int argc, char *argv[])
{
	srand((unsigned int)time(nullptr));

	const int root = 0;
	const int DATA_TAG = 0;
	double time;

	int procRank;
	int procNum;

	int rowsA = argc == 1 ? 3 : std::stoi(argv[1]);
	int colsA_rowsB = argc == 1 ? 4 : std::stoi(argv[2]);
	int colsB = argc == 1 ? 5 : std::stoi(argv[3]);

	int *matrixA = nullptr;
	int *matrixB = nullptr;
	int *transMatrixB = nullptr;
	int *matrixC = nullptr;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);
	MPI_Comm_size(MPI_COMM_WORLD, &procNum);


	if (procRank == root)
	{
		matrixA = new int[rowsA * colsA_rowsB];
		matrixB = new int[colsA_rowsB * colsB];
		transMatrixB = new int[colsB * colsA_rowsB];
		matrixC = new int[rowsA * colsB];

		//заполнение матриц случайными числами
		initMatrix(matrixA, rowsA, colsA_rowsB);
		initMatrix(matrixB, colsA_rowsB, colsB);

		transposeMatrix(matrixB, transMatrixB, colsA_rowsB, colsB);

		bool showMatrix = false;
		if (argc == 5 && std::string(argv[4]) == "true")
		{
			showMatrix = true;
		}

		//вывод матриц на экран
		if (showMatrix)
		{
			std::cout << "Matrix A with size " << rowsA << "x" << colsA_rowsB << std::endl;
			printMatrix(matrixA, rowsA, colsA_rowsB);
			std::cout << std::endl;

			std::cout << "Matrix B with size " << colsA_rowsB << "x" << colsB << std::endl;
			printMatrix(matrixB, colsA_rowsB, colsB);
			std::cout << std::endl;
		}
	}

	time = MPI_Wtime();

	//массив, содержащий кол-во посылаемых элементов для каждого процесса
	int *sizeOfSendRowsElem = procRank == root ? new int[procNum] : nullptr;
	int *sizeOfReceiveRowsElem = procRank == root ? new int[procNum] : nullptr;
	//массив, содержащий смещение
	int *displasmentsRowsElem = procRank == root ? new int[procNum] : nullptr;
	int *displasmentsReceive = procRank == root ? new int[procNum] : nullptr;

	//мастер-процесс определяет эти массивы
	if (procRank == root)
	{
		for (int i = 0; i < procNum; i++)
		{
			//определение числа передаваемых элеметов для процесса
			//если ранг процесса меньше чем число доп.строк, то добавим для текущего процесса одну доп.строку
			sizeOfSendRowsElem[i] = rowsA / procNum + (i < (rowsA % procNum) ? 1 : 0);

			sizeOfReceiveRowsElem[i] = sizeOfSendRowsElem[i] * colsB;

			//умножаем кол-во строк на размер одной строки
			//таким образом определяя кол-во передаваемых элементов процессу
			sizeOfSendRowsElem[i] *= colsA_rowsB;

			//определение смещений
			displasmentsRowsElem[i] = (i == 0) ? 0 :(displasmentsRowsElem[i - 1] + sizeOfSendRowsElem[i - 1]);
			displasmentsReceive[i] = i == 0 ? 0 : displasmentsReceive[i - 1] + sizeOfReceiveRowsElem[i - 1];
		}
	}
	else 
	{
		transMatrixB = new int[colsA_rowsB * colsB];
	}


	//определение числа получаемых строк для процесса
	//если ранг процесса меньше чем число доп.строк, то добавим для текущего процесса одну доп.строку
	int receiveCountRows = rowsA / procNum + (procRank < (rowsA % procNum) ? 1 : 0);

	int *bufA = new int[receiveCountRows * colsA_rowsB]{ 0 };
	int *bufC = new int[receiveCountRows * colsB]{ 0 };


	//рассылка строк и столбцов
	MPI_Scatterv(matrixA, sizeOfSendRowsElem, displasmentsRowsElem, MPI_INT, bufA, receiveCountRows * colsA_rowsB, MPI_INT, root, MPI_COMM_WORLD);
	MPI_Bcast(transMatrixB, colsB * colsA_rowsB, MPI_INT, root, MPI_COMM_WORLD);

	//скалярное произведение
	for (int i = 0; i < receiveCountRows; i++)
	{
		for (int j = 0; j < colsB; j++)
		{
			bufC[i * colsB + j] = scalarProduct(bufA + i * colsA_rowsB, transMatrixB + j * colsA_rowsB, colsA_rowsB);
		}
	}

	//сбор строк результирующей матрицы
	MPI_Gatherv(bufC, receiveCountRows * colsB, MPI_INT, matrixC, sizeOfReceiveRowsElem, displasmentsReceive, MPI_INT, root, MPI_COMM_WORLD);

	time = MPI_Wtime() - time;

	delete[] bufA;
	delete[] matrixB;
	delete[] bufC;

	if (procRank == root)
	{
		bool showMatrix = false;
		if (argc == 5 && std::string(argv[4]) == "true")
		{
			showMatrix = true;
		}

		std::cout.setf(std::ios::fixed);
		std::cout.precision(10);
		std::cout << "Matrix C = A * B with size " << rowsA << "x" << colsB << std::endl;

		if (showMatrix)
		{
			printMatrix(matrixC, rowsA, colsB);
		}

		std::cout << "Time = " << time << std::endl;
		delete[] matrixA;
		delete[] transMatrixB;
		delete[] matrixC;
	}

	MPI_Finalize();

	return 0;
}