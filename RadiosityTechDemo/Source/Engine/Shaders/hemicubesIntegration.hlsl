//------------------------------------------------------------------------------------------
// File: hemicubesIntegration.hlsl
//
// Compute Shaders para la segunda etapa del algoritmo de radiosidad en GPU.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constantes
//--------------------------------------------------------------------------------------

static const float PI = 3.14159265f;

static const uint HEMICUBE_FACE_SIZE_1 = HEMICUBE_FACE_SIZE - 1;	//para calcular el resto de la división por HEMICUBE_FACE_SIZE con &

static const uint HALF_HEMICUBE_FACE_SIZE = HEMICUBE_FACE_SIZE >> 1;
static const uint HALF_HEMICUBE_FACE_SIZE_1 = HALF_HEMICUBE_FACE_SIZE - 1;
static const uint HALF_HEMICUBE_FACE_SIZE_BITS = HEMICUBE_FACE_SIZE_BITS - 1;

static const uint FACE_SURFACE = (HEMICUBE_FACE_SIZE * HEMICUBE_FACE_SIZE);
static const uint HALF_FACE_SURFACE = FACE_SURFACE / 2;
static const uint HALF_FACE_SURFACE_1 = HALF_FACE_SURFACE - 1;


static const uint FACES_PER_ROW_1 = FACES_PER_ROW - 1;


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

//no todas las constantes se usan en un solo CS pero el tamaño minimo de un cb es 4x4 bytes
cbuffer cbGeneralConstants : register(b0)
{
	uint VertexID;
	float VertexWeight;
	uint SkippedVertices;
	uint NumVertices;
}
cbuffer cbUV : register(b1)
{
	float UV[64];
}


//--------------------------------------------------------------------------------------
// Texturas y Buffers
//--------------------------------------------------------------------------------------

//datos de entrada del IntegrationStepA
//esta textura tiene las 5 caras renderizadas de cada hemicubo en el siguiente orden +z +x -x +y -y
Texture2D<float4> HemicubeFaces : register(t0);	

//datos de entrada del IntegrationStepB
Buffer<float4> PartialIntegration : register(t0);

//datos de entrada del AddPasses
Buffer<float4> SumB1 : register(t0);
Buffer<float4> SumB2 : register(t1);


RWBuffer<float4> Output: register(u0);


//--------------------------------------------------------------------------------------
// Compute Shader. Primer paso de integración
//--------------------------------------------------------------------------------------

groupshared float3 Irradiances[2][GROUP_DIMENSION_X];
[numthreads(GROUP_DIMENSION_X, 1, 1)]
void IntegrationStepA(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	//1 Primer pixel

	//1.1 Construir índice del pixel para este hilo
	uint pixelLocation = GroupID.x * GROUP_DIMENSION_X + GroupThreadID.x;
	uint faceIndex = (pixelLocation >> HALF_FACE_SURFACE_BITS);

	//índice de la hemicube face (0 al 4) de este vértice. Notar que no hay warp divergence
	if(faceIndex >= 1) faceIndex = faceIndex-1;		

	//índice de cara en la textura principal
	uint faceNumber = (GroupID.y << 1) * NUM_HEMICUBE_FACES + faceIndex;

	//índice de fila en la textura principal para el primer vértice
	uint faceRow = (faceNumber >> FACES_PER_ROW_BITS);
	//índice de columna en la textura principal para el primer vértice
	uint faceColumn = (faceNumber & FACES_PER_ROW_1);

	//índice de fila en la textura principal para el segundo vértice
	uint faceRow2 = (faceNumber + NUM_HEMICUBE_FACES) >> FACES_PER_ROW_BITS;
	//índice de columna en la textura principal para el segundo vértice
	uint faceColumn2 = (faceNumber + NUM_HEMICUBE_FACES) & FACES_PER_ROW_1;

	//índice del pixel interno a una cara
	uint2 pixel = uint2(0,0);

	//los warps no divergen (tomando caras de 64x64 para el hemicubo se tiene que 1 warp lee 32*2 pixeles, 
	//y con 32 warps leemos una cara del hemicubo, excepto +z que es con 64 warps)
	if(faceIndex == 0)			//+z
		pixel = uint2(pixelLocation & HEMICUBE_FACE_SIZE_1, pixelLocation >> HEMICUBE_FACE_SIZE_BITS);
	else if(faceIndex == 1)		//+x
		pixel = uint2(pixelLocation & HALF_HEMICUBE_FACE_SIZE_1, (pixelLocation - FACE_SURFACE) >> HALF_HEMICUBE_FACE_SIZE_BITS);

	
	//1.2 Lectura de las radiancias. Se leen GROUP_DIMENSION_X posiciones consecutivas de alguna de las hemicube 
	//faces en un grupo. Coalescing access friendly.
	float3 vertex1PixelRadiance = HemicubeFaces.Load(uint3(pixel.x + faceColumn * HEMICUBE_FACE_SIZE, pixel.y + faceRow * HEMICUBE_FACE_SIZE, 0)).rgb;
	float3 vertex2PixelRadiance = HemicubeFaces.Load(uint3(pixel.x + faceColumn2 * HEMICUBE_FACE_SIZE, pixel.y + faceRow2 * HEMICUBE_FACE_SIZE, 0)).rgb;


	//1.3 Obtener irradiancia de cada pixel y guardarlo en memoria compartida

	//transformar coordenadas del pixel en la cara del hemicubo al rango [-1, 1]
	float u = (pixel.x / float(HEMICUBE_FACE_SIZE_1)) * 2.0f - 1.0f;
	float v = UV[pixel.y];

	//multiplicar radiancias por delta form factor correspondiente
	float deltaFF = 1.0f + u * u + v * v;
	deltaFF = deltaFF * deltaFF * PI;
	
	if(faceIndex == 0) {	//cara +z
		deltaFF = 1.0f/deltaFF;

		vertex1PixelRadiance *= deltaFF;
		vertex2PixelRadiance *= deltaFF;
	}
	else if(faceIndex == 1) {	//cara +x, -x
		deltaFF = (abs(u)/deltaFF);

		vertex1PixelRadiance *= deltaFF;
		vertex2PixelRadiance *= deltaFF;
	}

	//guardar irradiancias parciales en memoria compartida
	Irradiances[0][GroupThreadID.x] = vertex1PixelRadiance;
	Irradiances[1][GroupThreadID.x] = vertex2PixelRadiance;


	//2 Segundo pixel

	//2.1 Construir índice del segundo pixel para este hilo
	faceNumber -= faceIndex;
	pixelLocation += (HALF_FACE_SURFACE*3);
	faceIndex = (pixelLocation >> HALF_FACE_SURFACE_BITS) - 1;

	faceNumber += faceIndex;
	faceRow = (faceNumber >> FACES_PER_ROW_BITS);
	faceColumn = (faceNumber & FACES_PER_ROW_1);

	faceRow2 = (faceNumber + NUM_HEMICUBE_FACES) >> FACES_PER_ROW_BITS;
	faceColumn2 = (faceNumber + NUM_HEMICUBE_FACES) & FACES_PER_ROW_1;

	//el segundo pixel que procesamos en el hilo está en la segunda mitad del hemicubo
	if(faceIndex == 2)		//-x
		pixel = uint2((pixelLocation & HALF_HEMICUBE_FACE_SIZE_1) + HALF_HEMICUBE_FACE_SIZE, (pixelLocation - HALF_FACE_SURFACE*3) >> HALF_HEMICUBE_FACE_SIZE_BITS);
	else if(faceIndex == 3)		//+y
		pixel = uint2(pixelLocation & HEMICUBE_FACE_SIZE_1, ((pixelLocation - HALF_FACE_SURFACE*4) >> HEMICUBE_FACE_SIZE_BITS) + HALF_HEMICUBE_FACE_SIZE);
	else						//-y
		pixel = uint2(pixelLocation & HEMICUBE_FACE_SIZE_1, (pixelLocation - HALF_FACE_SURFACE*5) >> HEMICUBE_FACE_SIZE_BITS);


	//2.2 Lectura de las radiancias. Se leen GROUP_DIMENSION_X posiciones consecutivas de alguna de las hemicube 
	//faces en un grupo. Coalescing access friendly.
	vertex1PixelRadiance = HemicubeFaces.Load(uint3(pixel.x + faceColumn * HEMICUBE_FACE_SIZE, pixel.y + faceRow * HEMICUBE_FACE_SIZE, 0)).rgb;
	vertex2PixelRadiance = HemicubeFaces.Load(uint3(pixel.x + faceColumn2 * HEMICUBE_FACE_SIZE, pixel.y + faceRow2 * HEMICUBE_FACE_SIZE, 0)).rgb;


	//2.3 Obtener irradiancia de cada pixel y guardarlo en memoria compartida

	u = (pixel.x / float(HEMICUBE_FACE_SIZE_1)) * 2.0f - 1.0f;
	v = UV[pixel.y];

	deltaFF = 1.0f + u * u + v * v;
	deltaFF = deltaFF * deltaFF * PI;
	
	if(faceIndex == 2) {	//cara +x, -x
		deltaFF = (abs(u)/deltaFF);

		vertex1PixelRadiance *= deltaFF;
		vertex2PixelRadiance *= deltaFF;
	}
	else {    //cara +y, -y
		deltaFF = (abs(v)/deltaFF);

		vertex1PixelRadiance *= deltaFF;
		vertex2PixelRadiance *= deltaFF;
	}

	//guardar irradiancias parciales en memoria compartida
	Irradiances[0][GroupThreadID.x] += vertex1PixelRadiance;
	Irradiances[1][GroupThreadID.x] += vertex2PixelRadiance;
	GroupMemoryBarrierWithGroupSync();

	
	//3. Sumar mediante una parallel reduction. GROUP_DIMENSION_X = 96
	if(GroupThreadID.x < 48) {
		Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 48];
		Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 48];
	}
	GroupMemoryBarrierWithGroupSync();

	if(GroupThreadID.x < 24) {
		Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 24];
		Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 24];
		if(GroupThreadID.x < 12) {
			Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 12];
			Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 12];
			if(GroupThreadID.x < 6) {
				Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 6];
				Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 6];
				if(GroupThreadID.x < 3) {
					Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 3];
					Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 3];
					if(GroupThreadID.x < 1) {
						Irradiances[0][GroupThreadID.x] += Irradiances[0][GroupThreadID.x + 1];
						Irradiances[1][GroupThreadID.x] += Irradiances[1][GroupThreadID.x + 1];
					}
				}
			}
		}
	}

	//4. Los dos primeros hilos escriben al buffer de salida el resultado parcial para cada vértice
	if(GroupThreadID.x < 2) {
		Output[(SkippedVertices << DISPATCH_DIMENSION_X_BITS) + (((GroupID.y << 1) + GroupThreadID.x) << DISPATCH_DIMENSION_X_BITS) + GroupID.x] = 
			float4(Irradiances[GroupThreadID.x][0] + Irradiances[GroupThreadID.x][2], 0);
	}
}




//--------------------------------------------------------------------------------------
// Compute Shader. Segundo paso de integración
//--------------------------------------------------------------------------------------

groupshared float3 IrradiancesPartialSum[GROUP_DIMENSION_X_SECOND_SHADER];

[numthreads(GROUP_DIMENSION_X_SECOND_SHADER, 1, 1)]
void IntegrationStepB(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	//guardar en memoria compartida
	IrradiancesPartialSum[GroupThreadID.x] = PartialIntegration[(GroupID.x << DISPATCH_DIMENSION_X_BITS) + GroupThreadID.x].rgb + 
	                                         PartialIntegration[(GroupID.x << DISPATCH_DIMENSION_X_BITS) + GroupThreadID.x + 32].rgb;

	GroupMemoryBarrierWithGroupSync();

	if(GroupThreadID.x < 16) {
		IrradiancesPartialSum[GroupThreadID.x] += IrradiancesPartialSum[GroupThreadID.x + 16];
		if(GroupThreadID.x < 8)
			IrradiancesPartialSum[GroupThreadID.x] += IrradiancesPartialSum[GroupThreadID.x + 8];
		if(GroupThreadID.x < 4)
			IrradiancesPartialSum[GroupThreadID.x] += IrradiancesPartialSum[GroupThreadID.x + 4];
		if(GroupThreadID.x < 2)
			IrradiancesPartialSum[GroupThreadID.x] += IrradiancesPartialSum[GroupThreadID.x + 2];
		if(GroupThreadID.x < 1)
			IrradiancesPartialSum[GroupThreadID.x] += IrradiancesPartialSum[GroupThreadID.x + 1];
	}

	//el primer hilo escribe la irradiancia final del vértice al buffer de salida
	if (GroupThreadID.x == 0) {
		float3 irradiance = IrradiancesPartialSum[0];
		irradiance *= VertexWeight;

		Output[VertexID + GroupID.x] = float4(irradiance, 0);
	}
}



//--------------------------------------------------------------------------------------
// Compute Shader. Suma de irradiancias de dos iteraciones
//--------------------------------------------------------------------------------------

[numthreads(NUM_THREADS_FOR_FINAL_STEP, 1, 1)]
void AddPasses(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	const uint index = GroupThreadID.x + GroupID.x * NUM_THREADS_FOR_FINAL_STEP;
	if(index < NumVertices)
		Output[index] = SumB1[index] + SumB2[index];
}
