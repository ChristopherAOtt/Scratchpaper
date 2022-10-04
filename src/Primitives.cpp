#include "Primitives.hpp"


//------------------------------------------------------------------------------
// Vector Structures
//------------------------------------------------------------------------------
//--------------------------------------
// FVec2
//--------------------------------------
bool FVec2::isZero() const{
	return 
		(abs(x) < ARBITRARY_EPSILON) && 
		(abs(y) < ARBITRARY_EPSILON);
}

float FVec2::length() const{
	return sqrt(x*x + y*y);
}

FVec2 FVec2::normal() const{
	if(isZero()){
		return {0, 0}; // Return zero vector
	}

	float len = length();
	return {x / len, y / len};
}

FVec2 FVec2::operator*(float scalar) const{
	return {x * scalar, y * scalar};
}

float& FVec2::operator[](const int index){
	float* content_arr = &this->x;
	return content_arr[index];
}

float FVec2::operator[](const int index) const{
	const float* content_arr = &this->x;
	return content_arr[index];
}

FVec2 toFloatVector(const IVec2& vec){
	return {(float)vec.x, (float)vec.y};
}

void printPODStruct(const FVec2& vec){
	printf("{%f,%f}", vec.x, vec.y);
}

bool matchesWithinTolerance(const FVec2& v1, const FVec2& v2){
	for(int i = 0; i < 2; ++i){
		if(!matchesWithinTolerance(v1[i], v2[i])){
			return false;
		}
	}
	return true;
}


//--------------------------------------
// IVec2
//--------------------------------------
IVec2 IVec2::operator+(const IVec2& other) const{
	return {x + other.x, y + other.y};
}

IVec2 IVec2::operator-(const IVec2& other) const{
	return {x - other.x, y - other.y};
}

IVec2 IVec2::operator+=(const IVec2& other){
	IVec2 sum = *this + other;
	*this = sum;
	return sum;
}

IVec2 IVec2::operator-=(const IVec2& other){
	IVec2 diff = *this + other;
	*this = diff;
	return diff;
}

IVec2 IVec2::operator-() const{
	return {-x, -y};
}

IVec2 IVec2::operator*(int scalar) const{
	return {x * scalar, y * scalar};
}

IVec2 IVec2::operator*(float scalar) const{
	return {(int)(x * scalar), (int)(y * scalar)};
}

bool IVec2::operator==(const IVec2& other) const{
	return (x == other.x) && (y == other.y);
}

int& IVec2::operator[](const int index){
	int* content_arr = &this->x;
	return content_arr[index];
}

int IVec2::operator[](const int index) const{
	const int* content_arr = &this->x;
	return content_arr[index];
}

void printPODStruct(const IVec2& vec){
	printf("{%i,%i}", vec.x, vec.y);
}


//--------------------------------------
// FVec3
//--------------------------------------
bool FVec3::isZero() const{
	return 
		(abs(x) < ARBITRARY_EPSILON) && 
		(abs(y) < ARBITRARY_EPSILON) && 
		(abs(z) < ARBITRARY_EPSILON);
}

float FVec3::length() const{
	return sqrt(x*x + y*y + z*z);
}

float FVec3::dot(const FVec3& other) const{
	return x * other.x + y * other.y + z * other.z;
}

FVec3 FVec3::cross(const FVec3& other) const{
	//ATTRIBUTION: Sarrus's rule section on Wikipedia
	float v_x = y * other.z - z * other.y;
	float v_y = x * other.z - z * other.x;
	float v_z = x * other.y - y * other.x;

	return {v_x, -v_y, v_z};
}

FVec3 FVec3::normal() const{
	if(isZero()){
		return {0, 0, 0}; // Return zero vector
	}

	float len = length();
	return {x / len, y / len, z / len};
}

FVec3 FVec3::rotation(const FVec3 axis, float degrees) const{
	/*
	ATTRIBUTION: Rodrigues' rotation formula
	*/

	float radians = degreesToRadians(degrees);
	FVec3 v = *this; // Copy
	FVec3 a = axis.normal(); // Just to be sure

	FVec3 rot = v * cos(radians);
	rot += (a.cross(v)) * sin(radians);
	rot += a * (a.dot(v)) * (1 - cos(radians));
	return rot;
}

FVec3 FVec3::reflection(const FVec3 reflector) const{
	FVec3 norm = reflector.normal(); // Just to be sure
	FVec3 this_copy = *this;
	FVec3 output = this_copy - 2 * (this_copy.dot(norm)) * norm;
	return output;
}

FVec3 FVec3::operator+(const FVec3& other) const{
	return {x + other.x, y + other.y, z + other.z};
}

FVec3 FVec3::operator-(const FVec3& other) const{
	return {x - other.x, y - other.y, z - other.z};
}

FVec3 FVec3::operator+=(const FVec3& other){
	FVec3 sum = *this + other;
	*this = sum;
	return sum;
}

FVec3 FVec3::operator-=(const FVec3& other){
	FVec3 diff = *this - other;
	*this = diff;
	return diff;
}

FVec3 FVec3::operator-() const{
	return {-x, -y, -z};
}

FVec3 FVec3::operator*(float scalar) const{
	return {x * scalar, y * scalar, z * scalar};
}

FVec3 FVec3::operator/(float scalar) const{
	return {x / scalar, y / scalar, z / scalar};	
}

FVec3 operator*(float scalar, const FVec3 vec){
	/*
	Intentionally not a member function. Allows scalar * vector ordering.
	*/
	return vec * scalar;
}

FVec3 operator/(float scalar, const FVec3 vec){
	/*
	Intentionally not a member function. Allows scalar / vector ordering.
	*/
	return {scalar / vec.x, scalar / vec.y, scalar / vec.z};
}

float& FVec3::operator[](const int index){
	float* content_arr = &x;
	return content_arr[index];
}

float FVec3::operator[](const int index) const{ 
	const float* content_arr = &x;
	return content_arr[index];
}

FVec3 projection(FVec3 source, FVec3 target){
	FVec3 norm_target = target.normal();
	FVec3 norm_source = source.normal();
	return (norm_source.dot(norm_target)) * norm_target;
}

FVec3 toFloatVector(IVec3 vec){
	return {(float) vec.x, (float) vec.y, (float) vec.z};
}

FVec3 floatVector(int x, int y, int z){
	return {(float) x, (float) y, (float) z};
}

FVec3 hadamard(FVec3 a, FVec3 b){
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}

void printPODStruct(const FVec3& vec){
	printf("{%f,%f,%f}", vec.x, vec.y, vec.z);
}

bool matchesWithinTolerance(const FVec3& v1, const FVec3& v2){
	for(int i = 0; i < 3; ++i){
		if(!matchesWithinTolerance(v1[i], v2[i])){
			return false;
		}
	}
	return true;
}


//--------------------------------------
// IVec3
//--------------------------------------
IVec3 IVec3::operator+(const IVec3& other) const{
	return {x + other.x, y + other.y, z + other.z};
}

IVec3 IVec3::operator-(const IVec3& other) const{
	return {x - other.x, y - other.y, z - other.z};
}

IVec3 IVec3::operator+=(const IVec3& other){
	IVec3 sum = *this + other;
	*this = sum;
	return sum;
}

IVec3 IVec3::operator-=(const IVec3& other){
	IVec3 diff = *this + other;
	*this = diff;
	return diff;
}

IVec3 IVec3::operator-() const{
	return {-x, -y, -z};
}

IVec3 IVec3::operator*(int scalar) const{
	return {x * scalar, y * scalar, z * scalar};
}

bool IVec3::operator==(const IVec3& other) const{
	return (x == other.x) && (y == other.y) && (z == other.z);
}

bool IVec3::operator!=(const IVec3& other) const{
	return (x != other.x) || (y != other.y) || (z != other.z);
}

int& IVec3::operator[](const int index){
	int* content_arr = &x;
	return content_arr[index];
}

int IVec3::operator[](const int index) const{
	const int* content_arr = &x;
	return content_arr[index];
}

void printPODStruct(const IVec3& vec){
	printf("{%i,%i,%i}", vec.x, vec.y, vec.z);
}


//------------------------------------------------------------------------------
// Misc Math Structures
//------------------------------------------------------------------------------
//--------------------------------------
// FVec4
//--------------------------------------
float& FVec4::operator[](const int index){
	float* content_arr = &this->x;
	return content_arr[index];
}

float FVec4::operator[](const int index) const{
	const float* content_arr = &this->x;
	return content_arr[index];
}

void printPODStruct(const FVec4& vec){
	printf("{%f,%f,%f,%f}", vec.x, vec.y, vec.z, vec.w);
}


//--------------------------------------
// FMat4
//--------------------------------------
FMat4 FMat4::initIdentity(){
	/*

	*/

	FMat4 output;
	for(int y = 0; y < 4; ++y){
		for(int x = 0; x < 4; ++x){
			output[x][y] = (x == y);
		}
	}
	return output;
}

float FMat4::determinant() const{
	/*
	TODO: Figure out a non-awful way of doing this
	*/

	float* v = (float*)&r0;

	// Calculate 2x2 determinants
	float det2_1 = v[10]*v[15] - v[11]*v[14];
	float det2_2 = v[9]*v[15] - v[11]*v[13];
	float det2_3 = v[9]*v[14] - v[10]*v[13];
	float det2_4 = v[8]*v[15] - v[11]*v[12];
	float det2_5 = v[8]*v[14] - v[10]*v[11];
	float det2_6 = v[8]*v[13] - v[9]*v[12];

	// Calculate 3x3 determinants
	float det3_1 = v[5]*det2_1 - v[6]*det2_2 + v[7]*det2_3;
	float det3_2 = v[4]*det2_1 - v[6]*det2_4 + v[7]*det2_5;
	float det3_3 = v[4]*det2_2 - v[5]*det2_4 + v[7]*det2_6;
	float det3_4 = v[4]*det2_3 - v[5]*det2_5 + v[6]*det2_6;

	// Calculate this matrix's 4x4 determinant
	float det4_1 = v[0]*det3_1 - v[1]*det3_2 + v[2]*det3_3 - v[3]*det3_4;
	return det4_1;
}  

FMat4 FMat4::transposition() const{
	FMat4 output;

	const FVec4* rows = &r0;
	for(int i = 0; i < 4; ++i){
		for(int j = 0; j < 4; ++j){
			output[j][i] = rows[i][j];
		}
	}

	return output;
}

FMat4 FMat4::inverse() const{
	/*
	Notation uses a matrix of
		a, b, c, d
		e, f, g, h
		i, j, k, l
		m, n, o, p
	Since many of the 2x2 determinants repeat (18 unique instead of 48) I am
	calculating them first before doing the 3x3. 

	TODO: Figure out a non-awful way of doing this
	TODO: Handle non-invertible matrices
	*/

	// Aliases for cells
	float* v = (float*)&r0;
	float a = v[0];  float b = v[1];  float c = v[2];  float d = v[3];
	float e = v[4];  float f = v[5];  float g = v[6];  float h = v[7];
	float i = v[8];  float j = v[9];  float k = v[10]; float l = v[11];
	float m = v[12]; float n = v[13]; float o = v[14]; float p = v[15];

	// Calculate 2x2 determinants
	float det2_1  = k*p - o*l; float det2_2  = j*p - n*l;
	float det2_3  = j*o - n*k; float det2_4  = i*p - m*l;
	float det2_5  = i*o - m*k; float det2_6  = i*n - m*j;
	float det2_7  = g*p - o*h; float det2_8  = f*p - n*h;
	float det2_9  = f*o - n*g; float det2_10 = e*p - m*h;
	float det2_11 = e*o - m*g; float det2_12 = e*n - m*f;
	float det2_13 = g*l - k*h; float det2_14 = f*l - j*h;
	float det2_15 = f*k - j*g; float det2_16 = e*l - i*h;
	float det2_17 = e*k - i*g; float det2_18 = e*j - i*f;

	// Calculate 3x3 determinants
	float det3_1  = f*det2_1  - g*det2_2  + h*det2_3;
	float det3_2  = e*det2_1  - g*det2_4  + h*det2_5;
	float det3_3  = e*det2_2  - f*det2_4  + h*det2_6;
	float det3_4  = e*det2_3  - f*det2_5  + g*det2_6;
	float det3_5  = b*det2_1  - c*det2_2  + d*det2_3;
	float det3_6  = a*det2_1  - c*det2_4  + d*det2_5;
	float det3_7  = a*det2_2  - b*det2_4  + d*det2_6;
	float det3_8  = a*det2_3  - b*det2_5  + c*det2_6;
	float det3_9  = b*det2_7  - c*det2_8  + d*det2_9;
	float det3_10 = a*det2_7  - c*det2_10 + d*det2_11;
	float det3_11 = a*det2_8  - b*det2_10 + d*det2_12;
	float det3_12 = a*det2_9  - b*det2_11 + c*det2_12;
	float det3_13 = b*det2_13 - c*det2_14 + d*det2_15;
	float det3_14 = a*det2_13 - c*det2_16 + d*det2_17;
	float det3_15 = a*det2_14 - b*det2_16 + d*det2_18;
	float det3_16 = a*det2_15 - b*det2_17 + c*det2_18;

	// Calculate the determinant and (for now) halt if unable to invert.
	float det = (a*det3_1 - b*det3_2 + c*det3_3 - d*det3_4);
	float tolerance = 0.000001;
	assert(abs(det) > tolerance);

	// Multiply the adjoint by the inverse determinant
	float inv = 1.0 / det;
	return {
		{ inv*det3_1, -inv*det3_5,  inv*det3_9,  -inv*det3_13},
		{-inv*det3_2,  inv*det3_6, -inv*det3_10,  inv*det3_14},
		{ inv*det3_3, -inv*det3_7,  inv*det3_11, -inv*det3_15},
		{-inv*det3_4,  inv*det3_8, -inv*det3_12,  inv*det3_16},
	};
}

FMat4 FMat4::operator*(const FMat4 other) const{
	FMat4 output;

	const FVec4* rows = &r0;
	for(int i = 0; i < 4; ++i){  // this row
		for(int j = 0; j < 4; ++j){  // this col
			float sum = 0;
			for(int k = 0; k < 4; ++k){
				sum += rows[i][k] * other[k][j];
			}
			output[i][j] = sum;
		}
	}

	return output;
}

FMat4 FMat4::operator*(float scalar) const{
	FMat4 output;

	const float* source_rows = &r0.x;
	float* target_rows = &output.r0.x;
	for(int i = 0; i < 4 * 4; ++i){
		target_rows[i] = source_rows[i] * scalar;
	}

	return output;
}

FVec4 FMat4::operator*(const FVec4 vector) const{
	FVec4 output;

	const FVec4* rows = &r0;
	for(int i = 0; i < 4; ++i){
		float sum = 0;
		for(int k = 0; k < 4; ++k){
			sum += rows[i][k] * vector[k];
		}
		output[i] = sum;
	}

	return output;
}

FVec4& FMat4::operator[](const int index){
	FVec4* rows = &this->r0;
	return rows[index];
}

FVec4 FMat4::operator[](const int index) const{
	const FVec4* rows = &this->r0;
	return rows[index];
}

bool matchesWithinTolerance(FMat4 a, FMat4 b, float tolerance){
	/*
	Used to perform equality checks in spite of floating point imprecision
	*/

	bool is_match = true;
	for(int y = 0; y < 4; ++y){
		for(int x = 0; x < 4; ++x){
			float difference = abs(a[y][x] - b[y][x]);
			is_match &= (difference < tolerance);
		}
	}
	return is_match;
}

void printPODStruct(const FMat4& mat){
	printf("<FMat4>\n");
	for(int i = 0; i < 4; ++i){
		printf("\t");
		printPODStruct(mat[i]);
		printf("\n");
	}
	printf("</FMat4>");
}

//--------------------------------------
// Quaternion
// Reference: https://www.cprogramming.com/tutorial/3d/quaternions.html
//--------------------------------------
float Quat::magnitude() const{
	return sqrt(w*w + i*i + j*j + k*k);
}

Quat Quat::normal() const{
	float magnitude = this->magnitude();
	Quat output = {
		w / magnitude,
		i / magnitude,
		j / magnitude,
		k / magnitude,
	};
	return output;
}

Quat Quat::operator*(Quat other) const{
	return {
		w*other.w -  i*other.i - j*other.j - k*other.k,
		w*other.i + i*other.w +  j*other.k - k*other.j,
		w*other.j - i*other.k + j*other.w +  k*other.i,
		w*other.k + i*other.j - j*other.i + k*other.w
	};
}

Quat Quat::operator+(Quat other) const{
	return {w + other.w, i + other.i, j + other.j, k + other.k};
}

FMat4 Quat::asRotationMatrix() const{
	return {
		{1 - 2*j*j - 2*k*k,  2*i*j - 2*w*k,      2*i*k + 2*w*j,     0},
		{2*i*j + 2*w*k,      1 - 2*i*i - 2*k*k,  2*j*k + 2*w*i,     0},
		{2*i*k - 2*w*j,      2*j*k - 2*w*i,      1 - 2*i*i - 2*j*j, 0},
		{0,                  0,                  0,                 1},
	};
}

Quat initDefaultQuaternion(){
	/*
	Returns a quaternion that represents zero rotation
	*/

	return {1, 0, 0, 0};
}

Quat quatFromAxisAngle(FVec3 axis, float degrees){
	FVec3 norm_axis = axis.normal();
	float radian_angle = degreesToRadians(degrees);
	float half_angle = radian_angle / 2;
	return {
		cos(half_angle),
		norm_axis.x * sin(half_angle),
		norm_axis.y * sin(half_angle),
		norm_axis.z * sin(half_angle),
	};
}


//------------------------------------------------------------------------------
// Common-use Types
//------------------------------------------------------------------------------
//--------------------------------------
// String
//--------------------------------------
PODString PODString::init(const char* text){
	int init_string_len = strlen(text);
	assert(init_string_len <= MAX_LEN);

	PODString output = {};
	bzero(&output, sizeof(PODString));
	output.num_chars_remaining = MAX_LEN - init_string_len;
	strcpy(output.text, text);
	return output;
}

int PODString::len() const{
	return MAX_LEN - num_chars_remaining;
}

bool PODString::operator==(const PODString& other) const{
	if(num_chars_remaining == other.num_chars_remaining){
		// strcmp returns the difference between strings. 0 means they matched.
		return strcmp(text, other.text) == 0;
	}else{
		return false;
	}
}

bool PODString::operator==(const char* other) const{
	return strcmp(text, other) == 0;
}

void printPODStruct(const PODString& str){
	printf("<PODString '%s', NR: %i>", 
		str.text, str.num_chars_remaining);
}


//--------------------------------------
// Variant
//--------------------------------------
PODVariant PODVariant::init(PODVariant::DataType type, void* val){
	PODVariant output = {};
	bzero(&output, sizeof(PODVariant));

	// TODO: Verify that all union members have the same base ptr
	output.type = type;
	memcpy(&output.val_int, val, PODVariant::SIZE_BY_DATATYPE[type]);

	return output;
}

PODVariant PODVariant::init(){
	/*
	Returns a variant initialized into an invalid state
	*/

	PODVariant output = {};
	bzero(&output, sizeof(PODVariant));
	output.type = DATATYPE_INVALID;

	return output;
}


PODVariant PODVariant::init(bool boolean){
	return PODVariant::init(DATATYPE_BOOL, &boolean);
}

PODVariant PODVariant::init(float float_value){
	return PODVariant::init(DATATYPE_FLOAT, &float_value);
}

PODVariant PODVariant::init(Int32 integer){
	return PODVariant::init(DATATYPE_INT32, &integer);
}

PODVariant PODVariant::init(PODString string){
	return PODVariant::init(DATATYPE_STRING, &string);
}

PODVariant PODVariant::init(IVec2 vec){
	return PODVariant::init(DATATYPE_IVEC2, &vec);
}

PODVariant PODVariant::init(FVec2 vec){
	return PODVariant::init(DATATYPE_FVEC2, &vec);
}

PODVariant PODVariant::init(IVec3 vec){
	return PODVariant::init(DATATYPE_IVEC3, &vec);
}

PODVariant PODVariant::init(FVec3 vec){
	return PODVariant::init(DATATYPE_FVEC3, &vec);
}

bool PODVariant::operator==(const PODVariant& other){
	PODVariant var1 = *this;
	return memcmp(&var1, &other, sizeof(PODVariant));
}

void PODVariant::operator=(bool val){
	*this = PODVariant::init(val);
}

void PODVariant::operator=(Int32 val){
	*this = PODVariant::init(val);
}

void PODVariant::operator=(float val){
	*this = PODVariant::init(val);
}

void PODVariant::operator=(PODString val){
	*this = PODVariant::init(val);
}

void PODVariant::operator=(const char* val){
	*this = PODVariant::init(PODString::init(val));
}

void PODVariant::operator=(IVec2 vec){
	*this = PODVariant::init(vec);
}

void PODVariant::operator=(FVec2 vec){
	*this = PODVariant::init(vec);
}

void PODVariant::operator=(IVec3 vec){
	*this = PODVariant::init(vec);
}

void PODVariant::operator=(FVec3 vec){
	*this = PODVariant::init(vec);
}

void printPODStruct(const PODVariant& var){
	printf("<PODVariant ");
	switch(var.type){
		case PODVariant::DATATYPE_INVALID:
			printf("INVALID");
			break;
		case PODVariant::DATATYPE_BOOL:
			printf("bool %i", var.val_bool);
			break;
		case PODVariant::DATATYPE_INT32:
			printf("int %i", var.val_int);
			break;
		case PODVariant::DATATYPE_FLOAT:
			printf("float %f", var.val_float);
			break;
		case PODVariant::DATATYPE_STRING:
			printPODStruct(var.val_string);
			break;
		case PODVariant::DATATYPE_IVEC2:
			printPODStruct(var.val_ivec2);
			break;
		case PODVariant::DATATYPE_FVEC2:
			printPODStruct(var.val_fvec2);
			break;
		case PODVariant::DATATYPE_IVEC3:
			printPODStruct(var.val_ivec3);
			break;
		case PODVariant::DATATYPE_FVEC3:
			printPODStruct(var.val_fvec3);
			break;
		default:
			printf("UNKNOWN_TYPE");		
	}
	printf(">");
}

//--------------------------------------
// Dict
//--------------------------------------
PODDict PODDict::init(){
	PODDict output = {};
	bzero(&output, sizeof(PODDict));

	output.num_entries = 0;	

	return output;
}

Int32 PODDict::keyIndex(PODString str) const{
	for(Int32 i = 0; i < num_entries; ++i){
		if(str == table[i].key){
			return i;
		}
	}
	return -1;
}

Int32 PODDict::keyIndex(const char* key_text) const{
	return keyIndex(PODString::init(key_text));
}

Int32 PODDict::len() const{
	return num_entries;
}

PODVariant PODDict::getValue(PODString str) const{
	int retrieval_index = keyIndex(str);
	if(retrieval_index != -1){
		return table[retrieval_index].val;
	}else{
		return PODVariant::init();
	}
}

bool PODDict::update(KVPair pair){
	/*
	Returns true if an entry was added, false if one was modified.
	*/

	bool is_new_entry = false;
	int write_index = keyIndex(pair.key);
	if(write_index == -1){
		assert(num_entries != MAX_POD_DICT_KV_PAIRS);

		write_index = num_entries;
		is_new_entry = true;
	}

	table[write_index] = pair;
	++num_entries;
	return is_new_entry;
}

bool PODDict::update(const char* key_text, PODVariant value){
	return update({PODString::init(key_text), value});
}

bool PODDict::update(const char* key_text, bool value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, float value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, Int32 value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, const char* value_text){
	return update(key_text, PODVariant::init(PODString::init(value_text)));
}

bool PODDict::update(const char* key_text, IVec2 value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, FVec2 value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, IVec3 value){
	return update(key_text, PODVariant::init(value));
}

bool PODDict::update(const char* key_text, FVec3 value){
	return update(key_text, PODVariant::init(value));
}

PODDict::Iter PODDict::begin() const{
	return PODDict::Iter{0, this};
}

PODDict::Iter PODDict::end() const{
	return PODDict::Iter{num_entries, this};
}

void printPODStruct(const PODDict& dict){
	printf("<PODDict>\n");
	for(int i = 0; i < MAX_POD_DICT_KV_PAIRS; ++i){
		printf("\t[%03i]: ", i);

		if(i >= dict.num_entries){
			printf("UNUSED_RECORD\n");
		}else{
			KVPair pair = dict.table[i];

			printf("{");
			printPODStruct(pair.key);
			printf(" --> ");
			printPODStruct(pair.val);
			printf("}\n");
		}
	}
	printf("</PODDict>");
}

//--------------------------------------
// CellAddress
//--------------------------------------
bool CellAddress::operator==(const CellAddress& other) const{
	return (lod_power == other.lod_power) && (corner_addr == other.corner_addr);
}

void printPODStruct(const CellAddress& addr){
	printf("<CellAddress:"); 
	printPODStruct(addr.corner_addr); 
	printf("|lod_power=%i>", addr.lod_power);
}

//--------------------------------------
// ResourceHandle
//--------------------------------------
bool ResourceHandle::operator==(const ResourceHandle& other) const{
	return type == other.type && id == other.id;
}

void printPODStruct(const ResourceHandle& handle){
	printf("<ResourceHandle|%s|id=%u>", 
		RESOURCE_STRINGS[handle.type], handle.id);	
}

//--------------------------------------
// EntityHandle
//--------------------------------------
bool EntityHandle::operator==(const EntityHandle& other) const{
	return type_index == other.type_index && id == other.id;
}

void printPODStruct(const EntityHandle& handle){
	printf("<EntityHandle|type_index=%u|id=%u>", handle.type_index, handle.id);
}

//--------------------------------------
// PODHasher
//--------------------------------------
Bytes8 PODHasher::operator()(const IVec3& pos) const{
	return tripletHash(pos.x, pos.y, pos.z);
}

Bytes8 PODHasher::operator()(PODString str) const{
	/*
	ATTRIBUTION: http://www.cse.yorku.ca/~oz/hash.html
	The dbj2 hash function from Dan Berstein
	*/

	Bytes8 hash = 5381;
	int c;
	char* str_ptr = str.text;

	while((c = *str_ptr++)){
		hash = ((hash << 5) + hash) + c;  /* hash * 33 + c */
	}

	return hash;
}

Bytes8 PODHasher::operator()(const CellAddress& addr) const{
	/*
	WARNING: Garbage hash function. Done purely to get this compatible with unordered_map.
	TODO: Replace with better hash function.
	*/

	IVec3 coords = addr.corner_addr;
	Bytes8 output_hash = tripletHash(coords.x, coords.y, coords.z) ^ addr.lod_power;
	return output_hash;
}

Bytes8 PODHasher::operator()(const ResourceHandle& handle) const{
	// TODO: Verify that this works
	Bytes8 output = handle.type << 31;
	output |= handle.id;
	return output;
}

Bytes8 PODHasher::operator()(const EntityHandle& handle) const{
	// TODO: Verify that this works
	Bytes8 output = handle.type_index << 31;
	output |= handle.id;
	return output;
}

//------------------------------------------------------------------------------
// Hash namespace
//------------------------------------------------------------------------------
Hash::CoordHash Hash::modifiedSquirrelNoise(IVec3 coord, Bytes8 seed){
	/*
	ATTRIBUTION: Based on squirrel noise authored by Squirrel Eiserloh
	https://www.gdcvault.com/play/1024365/Math-for-Game-Programmers-Noise

		CoordHash mangled = toLinearValue(coord);
		mangled *= BIT_NOISE_1;
		mangled += seed;
		mangled ^= (mangled >> 8);
		mangled += BIT_NOISE_2;
		mangled ^= (mangled << 8);
		mangled *= BIT_NOISE_3;
		mangled ^= (mangled >> 8);

	This is a modified version of the above, which operated with 32-bit values.
	TODO: Actual statistical analysis to verify good distribution
	*/

	// First, linearize the coordinate
	CoordHash mangled = 0;
	mangled += coord.x;
	mangled += coord.y * LARGE_PRIME_1;
	mangled += coord.z * LARGE_PRIME_2;
	
	// Now mangle in some bit noise and seed info 
	mangled *= BIT_NOISE_s64_1;
	mangled += seed;
	mangled ^= (mangled >> 8);
	mangled += BIT_NOISE_s64_2;
	mangled ^= (mangled << 8);
	mangled *= BIT_NOISE_s64_3;
	mangled ^= (mangled >> 8);

	return mangled;
}




















