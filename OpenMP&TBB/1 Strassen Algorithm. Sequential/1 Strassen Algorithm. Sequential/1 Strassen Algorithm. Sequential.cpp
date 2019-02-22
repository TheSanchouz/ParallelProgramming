// 1 Strassen Algorithm. Sequential.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "Matrix.h"
#include <iostream>
#include <iomanip>

using namespace std;


int main()
{
	srand((unsigned int)time(nullptr));

	int size = 512;

	Matrix A(size, size);
	Matrix B(size, size);
	A.InitRandom();
	B.InitRandom();

	double t1 = omp_get_wtime();
	Matrix C = MultiplyStrassen(A, B);
	double t2 = omp_get_wtime();

	double t3 = omp_get_wtime();
	Matrix D = A * B;
	double t4 = omp_get_wtime();

	cout << "Time Strassen = " << t2 - t1 << endl;
	cout << "Time 3-cycle  = " << t4 - t3 << endl;

	return 0;
}