/**
 * Vaje07-OpenCL-Mandelbrotova-mnozica
 *
 * Algoritem za risanje Mandelbrotove mnozice, paraleliziran z OpenCL in
 * prilagojen za izvajanje na graficni kartici (nVidia K20).
 * Poleg izrisa mnozice, izracuna tudi ploscino mnozice (metoda stetja pikslov).
 * Uporabljena je knjiznica za delo z datotekami .pgm.
 *
 * Prevajaj z opcijami: "-lOpenCL -std=c99" (gcc).
 *
 * Porazdeljeni sistemi: 21.12.2015
 *
 * Avtor: Jure Jesensek
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <CL/cl.h>
#include "pgm.h"

#define SIZE_X			7000
#define SIZE_Y			4000
#define WORKGROUP_SIZE	(1024)
#define MAX_SOURCE_SIZE	20000

double dtime();

int main(void)
{
	double start, stop;

	cl_int ret;

	int size = 0;
	int *pSize = &size;

	// Branje datoteke
	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("kernel.cl", "r");
	if (!fp) {
		fprintf(stderr, "Error at opening kernel file\n");
		exit(1);
	}
	
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

	// Rezervacija pomnilnika
	PGMData slika;

	slika.height = SIZE_Y;
	slika.width = SIZE_X;
	slika.max_gray = 255;

	int vectorSize = slika.height * slika.width;	// velikost slike

	int *image = (int *)malloc(vectorSize * sizeof(int));	// slika

	// Podatki o platformi
	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
			// max. stevilo platform, kazalec na platforme, dejansko stevilo platform

	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,
						 device_id, &ret_num_devices);
			// izbrana platforma, tip naprave, koliko naprav nas zanima
			// kazalec na naprave, dejansko stevilo naprav

	// Kontekst
	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
			// kontekst: vkljucene platforme - NULL je privzeta, stevilo naprav,
			// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
			// dodatni parametri funkcije, stevilka napake

	// Ukazna vrsta
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
			// kontekst, naprava, INORDER/OUTOFORDER, napake

	// Delitev dela
	size_t local_item_size = WORKGROUP_SIZE;
	size_t num_groups = ((vectorSize - 1) / local_item_size + 1);
	size_t global_item_size = num_groups * local_item_size;

	// Alokacija pomnilnika na napravi
	// slika
	cl_mem image_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  vectorSize * sizeof(int), image, &ret);
	// pointer na int - velikost mnozice
	cl_mem size_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  1 * sizeof(int), pSize, &ret);
			// kontekst, nacin, koliko, lokacija na hostu, napaka

	// Priprava programa
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
													NULL, &ret);
			// kontekst, stevilo kazalcev na kodo, kazalci na kodo,
			// stringi so NULL terminated, napaka

	// Prevajanje
	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabniski argumenti

	// Scepec: priprava objekta
	cl_kernel kernel = clCreateKernel(program, "mandelbrot_px", &ret);
			// program, ime scepca, napaka

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
								sizeof(buf_size_t), &buf_size_t, NULL);

	// Scepec: argumenti
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&image_mem_obj);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&slika.height);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&slika.width);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&slika.max_gray);
	ret |= clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&size_mem_obj);
			// scepec, stevilka argumenta, velikost podatkov, kazalec na podatke

	// Zacetek merjenja izvajanja casa
	start = dtime();

	// scepec: zagon
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
									&global_item_size, &local_item_size, 0, NULL, NULL);
			// vrsta, scepec, dimenzionalnost, mora biti NULL,
			// kazalec na stevilo vseh niti, kazalec na lokalno stevilo niti,
			// dogodki, ki se morajo zgoditi pred klicem

	// Kopiranje rezultatov
	ret = clEnqueueReadBuffer(command_queue, image_mem_obj, CL_TRUE, 0,		// slika
								vectorSize * sizeof(int), image, 0, NULL, NULL);

	ret = clEnqueueReadBuffer(command_queue, size_mem_obj, CL_TRUE, 0,		// velikost lika
								sizeof(int), pSize, 0, NULL, NULL);
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej

	// Konec merjenja casa izvajanja
	stop = dtime();

	slika.image = image;

	// Shranjevanje rezultatov
	writePGM("mandelbrot.pgm", &slika);

	size = *pSize;

	double normSize = 3.5 * 2 * size / (slika.height * slika.width);	// normalizirana ploscina
	printf("Set size: %d / %d (= %f)\n", size, slika.height * slika.width, normSize);

	printf("\tTime: %f\n", stop - start);

	// Ciscenje
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(image_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	return 0;
}

double dtime()
{
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);

	return (tseconds);
}
