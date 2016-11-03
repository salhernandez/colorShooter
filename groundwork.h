#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "resource.h"
using namespace std;


struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

//*****************************************

//Added for sky sphere
// -EC
//////////////////////////////////////
bool LoadCatmullClark(LPCTSTR filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count)
{

	struct CatmullVertex
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 tex;
	};
	HANDLE file;
	std::vector<SimpleVertex> data;
	DWORD burn;

	file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SetFilePointer(file, 80, NULL, FILE_BEGIN);
	ReadFile(file, vertex_count, 4, &burn, NULL);

	for (int i = 0; i < *vertex_count; ++i)
	{
		CatmullVertex vertData;
		ReadFile(file, &vertData, sizeof(CatmullVertex), &burn, NULL);
		SimpleVertex sv;
		sv.Pos = vertData.pos;
		//sv.Normal = vertData.normal;
		sv.Tex = vertData.tex;
		data.push_back(sv);
	}

	D3D11_BUFFER_DESC desc = {
		sizeof(SimpleVertex) * *vertex_count,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0, 0,
		sizeof(SimpleVertex)
	};
	D3D11_SUBRESOURCE_DATA subdata = {
		&(data[0]), 0, 0
	};
	HRESULT hr = g_pd3dDevice->CreateBuffer(&desc, &subdata, ppVertexBuffer);

	if (FAILED(hr))
	{
		return false;
	}
	return true;
}
//END class for skysphere
///////////////////////////////

class ConstantBuffer
{
public:
	ConstantBuffer()
	{
		info = XMFLOAT4(1, 1, 1, 1);
		//colorChanger = false;
	}
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT4 info;
	XMFLOAT4 colorChanger;
};

class StopWatchMicro_
{
private:
	LARGE_INTEGER last, frequency;
public:
	StopWatchMicro_()
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&last);

	}
	long double elapse_micro()
	{
		LARGE_INTEGER now, dif;
		QueryPerformanceCounter(&now);
		dif.QuadPart = now.QuadPart - last.QuadPart;
		long double fdiff = (long double)dif.QuadPart;
		fdiff /= (long double)frequency.QuadPart;
		return fdiff*1000000.;
	}
	long double elapse_milli()
	{
		elapse_micro() / 1000.;
	}
	void start()
	{
		QueryPerformanceCounter(&last);
	}
};
class billboard
{
public:
	billboard()
	{
		position = XMFLOAT3(0, 0, 0);
		scale = 1;
		transparency = 1;
	}
	XMFLOAT3 position; //obvious
	float scale;		//in case it can grow
	float transparency; //for later use
	XMMATRIX get_matrix(XMMATRIX &ViewMatrix)
	{

		XMMATRIX view, R, T, S;
		view = ViewMatrix;
		//eliminate camera translation:
		view._41 = view._42 = view._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, view);//inverse rotation
		T = XMMatrixTranslation(position.x, position.y, position.z);
		S = XMMatrixScaling(scale, scale, scale);
		return S*R*T;
	}

	XMMATRIX get_matrix_y(XMMATRIX &ViewMatrix) //enemy-type
	{

	}
};

class bitmap
{

public:
	BYTE *image;
	int array_size;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	bitmap()
	{
		image = NULL;
	}
	~bitmap()
	{
		if (image)
			delete[] image;
		array_size = 0;
	}
	bool read_image(char *filename)
	{
		ifstream bmpfile(filename, ios::in | ios::binary);
		if (!bmpfile.is_open()) return FALSE;	// Error opening file
		bmpfile.read((char*)&bmfh, sizeof(BITMAPFILEHEADER));
		bmpfile.read((char*)&bmih, sizeof(BITMAPINFOHEADER));
		bmpfile.seekg(bmfh.bfOffBits, ios::beg);
		//make the array
		if (image)delete[] image;
		int size = bmih.biWidth*bmih.biHeight * 3;
		image = new BYTE[size];//3 because red, green and blue, each one byte
		bmpfile.read((char*)image, size);
		array_size = size;
		bmpfile.close();
		check_save();
		return TRUE;
	}
	BYTE get_pixel(int x, int y, int color_offset) //color_offset = 0,1 or 2 for red, green and blue
	{
		int array_position = x * 3 + y* bmih.biWidth * 3 + color_offset;
		if (array_position >= array_size) return 0;
		if (array_position < 0) return 0;
		return image[array_position];
	}
	void check_save()
	{
		ofstream nbmpfile("newpic.bmp", ios::out | ios::binary);
		if (!nbmpfile.is_open()) return;
		nbmpfile.write((char*)&bmfh, sizeof(BITMAPFILEHEADER));
		nbmpfile.write((char*)&bmih, sizeof(BITMAPINFOHEADER));
		//offset:
		int rest = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
		if (rest > 0)
		{
			BYTE *r = new BYTE[rest];
			memset(r, 0, rest);
			nbmpfile.write((char*)&r, rest);
		}
		nbmpfile.write((char*)image, array_size);
		nbmpfile.close();

	}
};
////////////////////////////////////////////////////////////////////////////////
//lets assume a wall is 10/10 big!
#define FULLWALL 2
#define HALFWALL 1
class wall
{
public:
	XMFLOAT3 position;
	int texture_no;
	int rotation; //0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
	wall()
	{
		texture_no = 0;
		rotation = 0;
		position = XMFLOAT3(0, 0, 0);
	}
	XMMATRIX get_matrix()
	{
		XMMATRIX R, T, T_offset;
		R = XMMatrixIdentity();
		T_offset = XMMatrixTranslation(0, 0, -HALFWALL);
		T = XMMatrixTranslation(position.x, position.y, position.z);
		switch (rotation)//0,1,2,3,4,5 ... facing to z, x, -z, -x, y, -y
		{
		default:
		case 0:	R = XMMatrixRotationY(XM_PI);		T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 1: R = XMMatrixRotationY(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 2:										T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 3: R = XMMatrixRotationY(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, HALFWALL); break;
		case 4: R = XMMatrixRotationX(XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
		case 5: R = XMMatrixRotationX(-XM_PIDIV2);	T_offset = XMMatrixTranslation(0, 0, -HALFWALL); break;
		}
		return T_offset * R * T;
	}
};
//********************************************************************************************
class level
{
private:
	bitmap leveldata;	//still private *NEW*
	vector<wall*> walls;						//all wall positions
	vector<ID3D11ShaderResourceView*> textures;	//all wall textures
	void process_level()
	{
		//we have to get the level to the middle:
		int x_offset = (leveldata.bmih.biWidth / 2)*FULLWALL;

		//lets go over each pixel without the borders!, only the inner ones
		for (int yy = 1; yy < (leveldata.bmih.biHeight - 1); yy++)
			for (int xx = 1; xx < (leveldata.bmih.biWidth - 1); xx++)
			{
				//wall information is the interface between pixels:
				//blue to something not blue: wall. texture number = 255 - blue
				//green only: floor. texture number = 255 - green
				//red only: ceiling. texture number = 255 - red
				//green and red: floor and ceiling ............
				BYTE red, green, blue;

				blue = leveldata.get_pixel(xx, yy, 0);
				green = leveldata.get_pixel(xx, yy, 1);
				red = leveldata.get_pixel(xx, yy, 2);

				if (blue > 0)//wall possible
				{
					int texno = 255 - blue;
					BYTE left_red = leveldata.get_pixel(xx - 1, yy, 2);
					BYTE left_green = leveldata.get_pixel(xx - 1, yy, 1);
					BYTE right_red = leveldata.get_pixel(xx + 1, yy, 2);
					BYTE right_green = leveldata.get_pixel(xx + 1, yy, 1);
					BYTE top_red = leveldata.get_pixel(xx, yy + 1, 2);
					BYTE top_green = leveldata.get_pixel(xx, yy + 1, 1);
					BYTE bottom_red = leveldata.get_pixel(xx, yy - 1, 2);
					BYTE bottom_green = leveldata.get_pixel(xx, yy - 1, 1);

					if (left_red>0 || left_green > 0)//to the left
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 3, texno);
					if (right_red>0 || right_green > 0)//to the right
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 1, texno);
					if (top_red>0 || top_green > 0)//to the top
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 2, texno);
					if (bottom_red>0 || bottom_green > 0)//to the bottom
						init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 0, texno);
				}
				if (red > 0)//ceiling
				{
					int texno = 255 - red;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 5, texno);
				}
				if (green > 0)//floor
				{
					int texno = 255 - green;
					init_wall(XMFLOAT3(xx*FULLWALL - x_offset, 0, yy*FULLWALL), 4, texno);
				}
			}
	}
	void init_wall(XMFLOAT3 pos, int rotation, int texture_no)
	{
		wall *w = new wall;
		walls.push_back(w);
		w->position = pos;
		w->rotation = rotation;
		w->texture_no = texture_no;
	}
public:
	bitmap *get_bitmap()//get method *NEW*
	{
		return &leveldata;
	}
	level()
	{
	}
	void init(char *level_bitmap)
	{
		if (!leveldata.read_image(level_bitmap))return;
		process_level();
	}
	bool init_texture(ID3D11Device* pd3dDevice, LPCWSTR filename)
	{
		// Load the Texture
		ID3D11ShaderResourceView *texture;
		HRESULT hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, filename, NULL, NULL, &texture, NULL);
		if (FAILED(hr))
			return FALSE;
		textures.push_back(texture);
		return TRUE;
	}
	ID3D11ShaderResourceView *get_texture(int no)
	{
		if (no < 0 || no >= textures.size()) return NULL;
		return textures[no];
	}
	XMMATRIX get_wall_matrix(int no)
	{
		if (no < 0 || no >= walls.size()) return XMMatrixIdentity();
		return walls[no]->get_matrix();
	}
	int get_wall_count()
	{
		return walls.size();
	}
	void render_level(ID3D11DeviceContext* ImmediateContext, ID3D11Buffer *vertexbuffer_wall, XMMATRIX *view, XMMATRIX *projection, ID3D11Buffer* dx_cbuffer)
	{
		//set up everything for the waqlls/floors/ceilings:
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		ImmediateContext->IASetVertexBuffers(0, 1, &vertexbuffer_wall, &stride, &offset);
		ConstantBuffer constantbuffer;
		constantbuffer.View = XMMatrixTranspose(*view);
		constantbuffer.Projection = XMMatrixTranspose(*projection);
		XMMATRIX wall_matrix, S;
		ID3D11ShaderResourceView* tex;
		//S = XMMatrixScaling(FULLWALL, FULLWALL, FULLWALL);
		S = XMMatrixScaling(1, 1, 1);
		for (int ii = 0; ii < walls.size(); ii++)
		{
			wall_matrix = walls[ii]->get_matrix();
			int texno = walls[ii]->texture_no;
			if (texno >= textures.size())
				texno = 0;
			tex = textures[texno];
			wall_matrix = wall_matrix;// *S;

			constantbuffer.World = XMMatrixTranspose(wall_matrix);

			ImmediateContext->UpdateSubresource(dx_cbuffer, 0, NULL, &constantbuffer, 0, 0);
			ImmediateContext->VSSetConstantBuffers(0, 1, &dx_cbuffer);
			ImmediateContext->PSSetConstantBuffers(0, 1, &dx_cbuffer);
			ImmediateContext->PSSetShaderResources(0, 1, &tex);
			ImmediateContext->Draw(6, 0);
		}
	}

	//wall detection
	bool isWalkable(XMFLOAT3 pos)
	{
		//do not ask for y, it doesnt move
		float x, z;
		x = pos.x;
		z = pos.z;


		//recalculate in pixel space
		BYTE colorMatch = leveldata.get_pixel(-(x) / 2 + 50.5, -(z) / 2 + .5, 0);
		if (colorMatch < 10)
		{
			return true;
		}
		return false;
	}
};



class camera
{
private:

public:
	int w, s, a, d;
	bool shift;//-SH
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	camera()
	{
		w = s = a = d = 0;
		shift = false;//-SH
		position = rotation = XMFLOAT3(0, 0, 0);
	}
	void animation(bitmap *leveldata)//bitmap in the argument NOW YOU CAN USE IT HERE!!! *NEW*
	{

		XMMATRIX R, T;
		R = XMMatrixRotationY(-rotation.y);

		XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
		XMVECTOR f = XMLoadFloat3(&forward);
		f = XMVector3TransformCoord(f, R);
		XMStoreFloat3(&forward, f);
		XMFLOAT3 side = XMFLOAT3(1, 0, 0);
		XMVECTOR si = XMLoadFloat3(&side);
		si = XMVector3TransformCoord(si, R);
		XMStoreFloat3(&side, si);

		XMFLOAT3 newPos = position;

		XMFLOAT3 det1, det2, det3, det4;

		//hold SHIFT for sprint -SH
		/////////////////////////////
		if (w)
		{
			if (shift) {// -SH
				newPos.x -= forward.x * 0.02;
				newPos.y -= forward.y * 0.02;
				newPos.z -= forward.z * 0.02;
			}

			else {
				newPos.x -= forward.x * 0.01;
				newPos.y -= forward.y * 0.01;
				newPos.z -= forward.z * 0.01;
			}
		}
		if (s)
		{
			if (shift) {// -SH
				newPos.x += forward.x * 0.02;
				newPos.y += forward.y * 0.02;
				newPos.z += forward.z * 0.02;
			}
			else {

				newPos.x += forward.x * 0.01;
				newPos.y += forward.y * 0.01;
				newPos.z += forward.z * 0.01;
			}
		}
		if (d)
		{
			if (shift) {// -SH
				newPos.x -= side.x * 0.015;
				newPos.y -= side.y * 0.015;
				newPos.z -= side.z * 0.015;

			}
			else {
				newPos.x -= side.x * 0.01;
				newPos.y -= side.y * 0.01;
				newPos.z -= side.z * 0.01;
			}
		}
		if (a)
		{
			if (shift) {// -SH
				newPos.x += side.x * 0.015;
				newPos.y += side.y * 0.015;
				newPos.z += side.z * 0.015;
			}
			else {
				newPos.x += side.x * 0.01;
				newPos.y += side.y * 0.01;
				newPos.z += side.z * 0.01;
			}

		}

		det1 = XMFLOAT3(newPos.x - 0.5, newPos.y, newPos.z + 0.5);
		det3 = XMFLOAT3(newPos.x - 0.5, newPos.y, newPos.z - 0.5);
		det2 = XMFLOAT3(newPos.x + 0.5, newPos.y, newPos.z + 0.5);
		det4 = XMFLOAT3(newPos.x + 0.5, newPos.y, newPos.z - 0.5);

		

		//COLLISION DETECTION
		BYTE colorMatch = leveldata->get_pixel(-(newPos.x) / 2 + 50.5, -(newPos.z) / 2 + .5, 0);
		if (colorMatch < 10)
		{
			position = newPos;
		}

		/*
		if (leveldata->isWalkable(newPos)) {
		position = newPos;
		}
		else {
		position = position;
		}
		*/

		//NEW WALL COLLISION DETECTION -ML
		//BETA VERSION
		///////////////////////////////////
		/*
		if (leveldata->isWalkable(det1)) {
			position = newPos;
		}
		else if (leveldata->isWalkable(det2)) {
			position = newPos;
		}
		else if (leveldata->isWalkable(det3)) {
			position = newPos;
		}
		else if (leveldata->isWalkable(det4)) {
			position = newPos;
		}
		else {
			position = position;
		}
		*/

		//END NEW WALL COLLISION
		////////////////////////////////
	}



	/*if (d)
	{
	rotation.y -= 0.005;
	}
	if (a)
	{
	rotation.y += 0.005;
	}*/




	XMMATRIX get_matrix(XMMATRIX *view)
	{
		XMMATRIX R, T;
		R = XMMatrixRotationY(rotation.y);
		T = XMMatrixTranslation(position.x, position.y, position.z);
		return T*(*view)*R;
	}
};

class bullet
{
public:
	XMFLOAT3 pos, imp;
	bullet()
	{
		imp = XMFLOAT3(0, 0, 0);
		pos = XMFLOAT3(0, 0, 0);
	}


	XMMATRIX getmatrix(float elapsed, XMMATRIX &view)
	{

		pos.x = pos.x + imp.x *(elapsed / 100000.0);
		pos.y = pos.y + imp.y *(elapsed / 100000.0);
		pos.z = pos.z + imp.z *(elapsed / 100000.0);

		XMMATRIX R, T;
		R = view;
		R._41 = R._42 = R._43 = 0.0;
		XMVECTOR det;
		R = XMMatrixInverse(&det, R);
		T = XMMatrixTranslation(pos.x, pos.y, pos.z);

		return R * T;
	}
};