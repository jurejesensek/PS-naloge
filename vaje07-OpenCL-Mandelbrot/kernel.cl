// kernel
__kernel void mandelbrot_px(__global int *image, 
							int height, int width, 
							int max_gray, __global int *size)
{
	int heightLoc = height;
	int widthLoc = width;
	int max_grayLoc = max_gray;
	
	int length = heightLoc * widthLoc;

	// globalni indeks elementa	
	int idx = get_global_id(0);
	
	if (idx < length) {
		int max_iteration = 1000;   // stevilo iteracij
		int sizeLocal = 0;			// velikost lika

		int row = idx / widthLoc;
		int col = idx % widthLoc;
	
		float x0 = (float)col / widthLoc * (float)3.5 - (float)2.5;	// zacetna vrednost
		float y0 = (float)row / heightLoc * (float)2.0 - (float)1.0;
		float x = 0.0;
		float y = 0.0;
		int iter = 0;
		
		// ponavljamo, dokler ne izpolnemo enega izmed pogojev
		while ((x * x + y * y <= 4) && (iter < max_iteration)) {
			float xtemp = x * x - y * y + x0;
			y = 2 * x * y + y0;
			x = xtemp;
			iter++;
		}
		
		// pobarvamo piksel z ustrezno barvo
		int color = (int)(iter / (float)max_iteration * (float)max_grayLoc);
		image[idx] = max_grayLoc - color;
		
		// velikost mnozice
		if (iter > 950) {
			atomic_inc(size);
		}
	}
}