
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include "imageloader.h"
#include "vec3f.h"
#include "color.h"
#endif




//static GLfloat spin, spin2 = 0.0;
float angle = 0;
float sudutk = 30.0f;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = 50;
static int viewy = 24;
static int viewz = 80;
int a=0, b=0, c=0, d=0;
float rot = 0;


//train 2D
//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;
}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {

	float scale = 150.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}
void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);


	_terrain = loadTerrain("Terrain.bmp", 20);
	_terrainTanah = loadTerrain("Jalan.bmp", 20);
	_terrainAir = loadTerrain("heightmapAir.bmp", 8);

	//binding texture

}

//BANGUN RUMAH JOGLO
void lantai1()
{
 glPushMatrix();
 glTranslatef(-10.0f, -5.3f, 13.5f);
 glScalef(0.5,0.5,0.5);
 glBegin(GL_QUADS);        // Draw The Cube Using quads
 glColor3f(1.1f,1.0f,1.1f);    // Color Blue
 glVertex3f( 48.0f, 1.0f,-40.0f);    // Top Right Of The Quad (Top)
 glVertex3f(-48.0f, 1.0f,-40.0f);    // Top Left Of The Quad (Top)
 glVertex3f(-48.0f, 1.0f, 40.0f);    // Bottom Left Of The Quad (Top)
 glVertex3f( 48.0f, 1.0f, 40.0f);    // Bottom Right Of The Quad (Top)
 glColor3f(1.0f,1.0f,1.0f);    // Color Orange
 glVertex3f( 48.0f,-0.0f, 35.0f);    // Top Right Of The Quad (Bottom)
 glVertex3f(-48.0f,-0.0f, 35.0f);    // Top Left Of The Quad (Bottom)
 glVertex3f(-48.0f,-0.0f,-35.0f);    // Bottom Left Of The Quad (Bottom)
 glVertex3f( 48.0f,-0.0f,-35.0f);    // Bottom Right Of The Quad (Bottom)
 glColor3f(1.0f,1.0f,1.0f);    // Color Red
 glVertex3f( 48.0f, 1.0f, 40.0f);    // Top Right Of The Quad (Front)
 glVertex3f(-48.0f, 1.0f, 40.0f);    // Top Left Of The Quad (Front)
 glVertex3f(-48.0f,-1.0f, 40.0f);    // Bottom Left Of The Quad (Front)
 glVertex3f( 48.0f,-1.0f, 40.0f);    // Bottom Right Of The Quad (Front)
 glColor3f(1.0f,1.0f,1.0f);    // Color Yellow
 glVertex3f( 48.0f,-1.0f,-40.0f);    // Top Right Of The Quad (Back)
 glVertex3f(-48.0f,-1.0f,-40.0f);    // Top Left Of The Quad (Back)
 glVertex3f(-48.0f, 1.0f,-40.0f);    // Bottom Left Of The Quad (Back)
 glVertex3f( 48.0f, 1.0f,-40.0f);    // Bottom Right Of The Quad (Back)
 glColor3f(1.0f,1.0f,1.0f);    // Color Blue
 glVertex3f(-48.0f, 1.0f, 40.0f);    // Top Right Of The Quad (Left)
 glVertex3f(-48.0f, 1.0f,-40.0f);    // Top Left Of The Quad (Left)
 glVertex3f(-48.0f,-1.0f,-40.0f);    // Bottom Left Of The Quad (Left)
 glVertex3f(-48.0f,-1.0f, 40.0f);    // Bottom Right Of The Quad (Left)
 glColor3f(1.0f,1.0f,1.0f);    // Color Violet
 glVertex3f( 48.0f, 1.0f,-40.0f);    // Top Right Of The Quad (Right)
 glVertex3f( 48.0f, 1.0f, 40.0f);    // Top Left Of The Quad (Right)
 glVertex3f( 48.0f,-1.0f, 40.0f);    // Bottom Left Of The Quad (Right)
 glVertex3f( 48.0f,-1.0f,-40.0f);    // Bottom Right Of The Quad (Right)

 glEnd();

    glPopMatrix();

}

void umpak1(){
		glPushMatrix();
        //glTranslatef(12.0f,-4.7f,-4.0f);
        glColor4f(1.0, 1.0, 1.0, 1.0);
        glScalef(0.5,0.5,0.5);
        glutSolidCube(3.0f);
        glPopMatrix();

    }
//sokoguru(tiang)
void sokoguru(){
       glPushMatrix();
       glColor4f(1.0, 0.4, 0.0, 1.0);
       glScalef(0.5,0.5,0.5);
       glutSolidCube(2.0f);
       glPopMatrix();
}
//sokoguru(tiang)
void soko(){
       glPushMatrix();
       glColor4f(1.0, 0.4, 0.0, 1.0);
       glScalef(0.5,9.0,0.5);
       glutSolidCube(2.0f);
       glPopMatrix();
}

//sokoguru(tiang)
void soko_luar(){
       glPushMatrix();
       glColor4f(1.0, 0.4, 0.0, 1.0);
       glScalef(0.5,17.0,0.5);
       glutSolidCube(2.0f);
       glPopMatrix();
}
//sokoguru(tiang)
void soko_dalam(){
       glPushMatrix();
       glColor4f(1.0, 0.4, 0.0, 1.0);
       glScalef(0.5,13.0,0.5);
       glutSolidCube(2.0f);
       glPopMatrix();
}
void soko_dalamdepan(){
       glPushMatrix();
       glColor4f(1.0, 0.4, 0.0, 1.0);
       glScalef(0.5,12.0,0.5);
       glutSolidCube(2.0f);
       glPopMatrix();
}
//blandar_sunduk
void blandar_sunduk(){
        glPushMatrix();
        glColor4f(1.0, 0.7, 0.0, 1.0);
        glScalef(0.5,0.5,0.5);
        glutSolidCube(2.5);
        glPopMatrix();
}

void kandang_ayam(){
	glPushMatrix();
	glColor4f(1.0, 0.7, 0.0, 1.0);
    glScalef(0.5,0.1,0.5);
	glutSolidCube(20);
	glPopMatrix();

}

void tiang_kandang(){
	glPushMatrix();
	glColor4f(0.0, 0.0, 0.0, 0.0);
    glScalef(0.1,2.5,0.1);
	glutSolidCube(5);
	glPopMatrix();

}
void atap_kandang(){
	glPushMatrix();
	glColor3f(Red);
	glScalef(0.5,0.06,0.5);
	glutSolidCube(20);
	glPopMatrix();

}

void atap()
{
	glPushMatrix();
	glBegin(GL_POLYGON);
    //atap depan bawah
	glColor3f(Red);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glVertex3f( 4.8f, 11.9f,34.0f);    // Top Right Of The Quad (Front)
    glVertex3f(-21.0f,11.9f,34.0f);    // Top Left Of The Quad (Front)
    glVertex3f(-48.0f,5.6f, 58.5f);    // Bottom Left Of The Quad (Front)
    glVertex3f( 32.0f,5.6f, 58.5f);
    glPopMatrix();
    glEnd();

    //atap kanan bawah
    glBegin(GL_POLYGON);
    glColor3f(Red);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glVertex3f(-21.0f, 11.9f, 34.0f);    // Top Right Of The Quad (Left)
    glVertex3f(-21.0f,11.9f,13.0f);    // Top Left Of The Quad (Left)
    glVertex3f(-48.0f,5.6f,-10.6f);    // Bottom Left Of The Quad (Left)
    glVertex3f(-48.0f,5.6f, 58.5f);    // Bottom Right Of The Quad (Left)
    glEnd();
    //atap belakang bawah
    glBegin(GL_POLYGON);
    glColor3f(Red);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glVertex3f( 32.0f,5.6f,-10.6f);    // Top Right Of The Quad (Back)
     glVertex3f(-48.0f,5.6f,-10.6f);    // Top Left Of The Quad (Back)
     glVertex3f(-21.0f, 11.9f,13.0f);    // Bottom Left Of The Quad (Back)
     glVertex3f( 4.8f, 11.9f,13.0f);    // Bottom Right Of The Quad (Back)glEnd();
    glEnd();
    //atap kiri bawah
    glBegin(GL_POLYGON);
    glColor3f(Red);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glVertex3f( 4.65f, 11.9f,13.0f);    // Top Right Of The Quad (Right)
     glVertex3f( 4.65f, 11.9f,34.0f);    // Top Left Of The Quad (Right)
     glVertex3f( 32.0f,5.6f, 58.5f);    // Bottom Left Of The Quad (Right)
     glVertex3f( 32.0f,5.6f,-10.6f);    // Bottom Right Of The Quad (Right)
    glEnd();
     glPopMatrix();

}
void atap_atas(void){
	glPushMatrix();
	    glBegin(GL_POLYGON);        // Draw The Cube Using quads
	    //atap depan atas
	    glColor3f(Red);    // Color Blue
	    glVertex3f( -3.3f,22.5f,24.0f);    // Top Right Of The Quad (Front)
	    glVertex3f(-12.8f,22.5f,24.0f);    // Top Left Of The Quad (Front)
	    glVertex3f(-21.2f,11.7f, 34.2f);    // Bottom Left Of The Quad (Front)
	    glVertex3f( 4.8f,11.7f, 34.2f);
	    glEnd();
	    //atap kanan atas
	    glBegin(GL_POLYGON);
	    glColor3f(Red);
	    glVertex3f(-12.5f, 22.5f,24.0f);    // Top Right Of The Quad (Left)
	    glVertex3f(-12.5f,22.5f,23.1f);    // Top Left Of The Quad (Left)
	    glVertex3f(-21.2f,11.7f,12.7f);    // Bottom Left Of The Quad (Left)
	    glVertex3f(-21.2f,11.7f, 34.2f);    // Bottom Right Of The Quad (Left)
	    glEnd();
	    //atap belakang atas
	    glBegin(GL_POLYGON);
	    glColor3f(Red);
	    glVertex3f( 4.8f,11.7f,12.7f);    // Top Right Of The Quad (Back)
	    glVertex3f(-21.2f,11.7f,12.7f);    // Top Left Of The Quad (Back)
	    glVertex3f(-12.8f, 22.5f,23.1f);    // Bottom Left Of The Quad (Back)
	    glVertex3f( -3.3f, 22.5f,23.1f);    // Bottom Right Of The Quad (Back)glEnd();
	    glEnd();
	    //atap kiri atas
	    glBegin(GL_POLYGON);
	    glColor3f(Red);
	    glVertex3f( -3.3f, 22.5f,23.1f);    // Top Right Of The Quad (Right)
	    glVertex3f( -3.3f, 22.5f,24.0f);    // Top Left Of The Quad (Right)
	    glVertex3f( 4.8f,11.7f, 34.2f);    // Bottom Left Of The Quad (Right)
	    glVertex3f( 4.8f,11.7f,12.7f);    // Bottom Right Of The Quad (Right)
	    glEnd();
	    glPopMatrix();



}

void pohon(void){
	//batang
		GLUquadricObj *pObj;
		pObj =gluNewQuadric();
		gluQuadricNormals(pObj, GLU_SMOOTH);

		glPushMatrix();
		glColor3ub(104,70,14);
		glRotatef(270,1,0,0);
		gluCylinder(pObj, 4, 0.7, 30, 25, 25);
		glPopMatrix();
}
void ranting(void){
			GLUquadricObj *pObj;
			pObj =gluNewQuadric();
			gluQuadricNormals(pObj, GLU_SMOOTH);
			glPushMatrix();
			glColor3ub(104,70,14);
			glTranslatef(0,27,0);
			glRotatef(330,1,0,0);
			gluCylinder(pObj, 0.6, 0.1, 15, 25, 25);
			glPopMatrix();
			}
void daun(void){
	//daun
				glPushMatrix();
				glColor3ub(18,118,13);
				glScaled(5, 5, 5);
				glTranslatef(0,7,3);
				glutSolidDodecahedron();
				glPopMatrix();

}

void jalan_setapak(void){
				glPushMatrix();
				glColor3f(DimGray);
				glScalef(0.2,0.08,0.5);
				glutSolidCube(15);
				glPopMatrix();
}
void pagar_rumah(void){
		       glPushMatrix();
	           glColor3f(DarkWood);
	           glScalef(0.5,2.3,0.5);
	           glutSolidCube(2.0f);
	           glPopMatrix();

}
void pagar_rumah_atas(void){
		       glPushMatrix();
	           glColor3f(DarkWood);
	           glutSolidCube(2.0f);
	           glPopMatrix();

}

void pagar_luar(){
	glPushMatrix();
	glColor3f(1.0,1.0,1.0);
	glutSolidCube(3.0f);
	glPopMatrix();


}
//bawah gapura
void bawah_gapura1(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.6,0.1,0.6);
	glutSolidCube(6);
	glPopMatrix();

}
//atas_gapura1
void atas_gapura(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.2,0.5);
	glutSolidCube(3);
	glPopMatrix();
}
void atas_gapura2(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(5.0);
	glPopMatrix();
}
void atas_gapura3(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(4);
	glPopMatrix();
}
void atas_gapura4(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(3.0);
	glPopMatrix();
}
void atas_gapura5(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(2.0);
	glPopMatrix();
}
void atas_gapura6(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(1.0);
	glPopMatrix();
}
void atas_gapura7(){
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.5);
    glScalef(0.5,0.05,0.5);
	glutSolidCube(0.5);
	glPopMatrix();
}

//pagar_gapura
void pagar_gapura(void){
		       glPushMatrix();
	           glColor4f(0.5, 0.4, 0.5, 0.5);
	           glScalef(0.5,2.3,0.5);
	           glutSolidCube(0.7f);
	           glPopMatrix();

}
void pagar_gapura_atas(void){
		       glPushMatrix();
	           glColor4f(0.5, 0.4, 0.5, 0.5);
	           glutSolidCube(1.0f);
	           glPopMatrix();
}
void lampu (void){
                glPushMatrix();
                glColor4f(1.0, 1.0, 0.0, 0.0);
                glutSolidSphere(2.5,6.5,3);
                glPopMatrix();
}
void tiang_lampu(void){
               glPushMatrix();
	           glColor4f(0.5, 0.4, 0.5, 0.5);
	           glutSolidCube(1.0f);
	           glPopMatrix();
}

void kursi_kaki(void){
    glPushMatrix();
    glColor4f(1, 1, 1, 1);
    glutSolidCube(1.0f);
    glPopMatrix();
}
void kursi_atas(void){
    glPushMatrix();
    glColor4f(1.0f, 0.4f, 0.1f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();
}
void sandaran(void){
    glPushMatrix();
    glColor4f(1.0f, 0.4f, 0.1f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();
}
void tiang(void){
	//silinder
		GLUquadricObj *pObj;
		pObj =gluNewQuadric();
		gluQuadricNormals(pObj, GLU_SMOOTH);

		glPushMatrix();
		glColor3ub(104,70,14);
		glRotatef(270,1,0,0);
		gluCylinder(pObj, 1, 1, 11.7, 25, 25);
		glPopMatrix();
}
void tutup_tiang(void){
	//silinder
		GLUquadricObj *pObj;
		pObj =gluNewQuadric();
		gluQuadricNormals(pObj, GLU_SMOOTH);

		glPushMatrix();
		glColor3ub(0.5,0.5,0.5);
		glRotatef(270,1,0,0);
		gluCylinder(pObj, 1.3, 1.3, 1, 25, 25);
		glPopMatrix();
}
void tiang_kolam(void){
	//silinder
			GLUquadricObj *pObj;
			pObj =gluNewQuadric();
			gluQuadricNormals(pObj, GLU_SMOOTH);

			glPushMatrix();
			glColor3f(NeonBlue);
			glRotatef(270,1,0,0);
			gluCylinder(pObj, 1.3, 1.3, 6, 25, 25);
			glPopMatrix();

}
void awan(void){
glPushMatrix();
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3ub(153, 223, 255);
glutSolidSphere(10, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(10,70,-50);
glutSolidSphere(5, 50, 90);
glPopMatrix();
glPushMatrix();
glTranslatef(-2,65,-60);
glutSolidSphere(10, 50, 80);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,63,-63);
glutSolidSphere(7, 50, 70);
glPopMatrix();
glPushMatrix();
glTranslatef(6,68,-65);
glutSolidSphere(7, 120, 50);
glPopMatrix();
}




//unsigned int LoadTextureFromBmpFile(char *filename);

void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx-110, viewy+20, viewz+20, 0.0, 10.0, 5.0, 0.0, 1.0, 0.0);

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.0f, 0.5f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah, 0.0f, 0.0f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

    //lantai joglo
    glPushMatrix();
    glTranslatef(18.0, 9.0,-35.0);
    glScaled(1.7, 1.7, 1.7);
    lantai1();

    // Umpak Mulai
    //---------------------------------------------------
    //umpak_belakang
    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 1, -42);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }


    //umpak_depan
    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 1, 18);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }

    //umpak kiri
    for (int i=0; i <= 3; i++ ){
    int j=20;
    glPushMatrix();
    glTranslated(-36, 1, -42+j*i);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }

    //umpak kanan
    for (int i=0; i <= 3; i++ ){
    int j=20;
    glPushMatrix();
    glTranslated(36, 1, -42+j*i);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }


    //=====UMPAK TENGAH====//
    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 1, -22);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }

    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 1, -2);
    glScaled(1.7, 1.7, 1.7);
    umpak1();
    }

    //---------------------------------------------------
    // Umpak Selesai

    //====SOKOGURU(tiang)======//
    //belakang
    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 12, -42);
    glScaled(1.5, 27.0, 1.5);
    sokoguru();
    }

    //depan
    for (int i=0; i <= 3; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-36+j*i, 12, 18);
    glScaled(1.5, 27.0, 1.5);
    sokoguru();
    }

    //kiri
    for (int i=0; i <= 3; i++ ){
    int j=20;
    glPushMatrix();
    glTranslated(-36, 12, -42+j*i);
    glScaled(1.5, 27.0, 1.5);
    sokoguru();
    }

    //kanan
    for (int i=0; i <= 3; i++ ){
    int j=20;
    glPushMatrix();
    glTranslated(36, 12, -42+j*i);
    glScaled(1.5, 27.0, 1.5);
    sokoguru();
    }
    //tengah belakang
    for (int i=0; i < 2; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-12+j*i, 16, -22);
    glScaled(1.5, 28.0, 1.5);
    sokoguru();
    }

    //tengah depan
    for (int i=0; i < 2; i++ ){
    int j=24;
    glPushMatrix();
    glTranslated(-12+j*i, 16, -2);
    glScaled(1.5, 28.0, 1.5);
    sokoguru();
    }
   //==========================================//
    //sokoguru selesai

    //blandar_dan_sunduk
    //===========================================//
    //level1
    //kanan kiri
    for (int i=0; i < 2; i++ ){
        int j=24;
        glPushMatrix();
        glTranslated(-12+j*i, 28, -12);
        glScaled(1.2, 1.2, 15.0);
        blandar_sunduk();
        }

    //depan belakang
    for (int i=0; i < 2; i++ ){
            int j=20;
            glPushMatrix();
            glTranslated(-0, 28, -22+j*i);
            glScaled(18.0, 1.2, 1.2);
            blandar_sunduk();
            }
    //selesai level1

    //level2
    //kanan kiri
        for (int i=0; i < 2; i++ ){
            int j=24;
            glPushMatrix();
            glTranslatef(-12+j*i, 30.8, -12);
            glScaled(0.8, 0.5, 21.0);
            blandar_sunduk();
            }

        //depan belakang
        for (int i=0; i < 2; i++ ){
                int j=20;
                glPushMatrix();
                glTranslated(-0, 30.2, -22+j*i);
                glScaled(23.0, 0.5, 0.8);
                blandar_sunduk();
                }
        //selesai level2

        //plus
         glPushMatrix();
         glTranslatef(-0, 29.3, -12);
         glScaled(23.0, 0.8, 1.0);
         blandar_sunduk();

         glPushMatrix();
         glTranslatef(-0, 29.5, -12);
         glScaled(0.8, 1.0, 23.0);
         blandar_sunduk();

        //plus selesai
            //===========================================//

        //Ander
        //===============================================//
         glPushMatrix();
         glTranslated(-0, 35, -12);
         glScaled(1.0, 13.0, 1.0);
         sokoguru();

         //puncak
         glPushMatrix();
         glTranslatef(-0, 41.8, -12);
         glScaled(7.0, 0.6, 0.6);
         blandar_sunduk();
         //

         //kotak_luar_depan_belakang
         for (int i=0; i < 2; i++ ){
         int j=67.5;
         glPushMatrix();
         glTranslatef(-0, 25, -45.1+j*i);
         glScalef(63.5, 1.0, 1.0);
         blandar_sunduk();
         }
         //

         //kotak_luar_kanan_kiri
         for (int i=0; i < 2; i++ ){
         int j=78;
         glPushMatrix();
         glTranslated(-39+j*i, 25, -12);
         glScaled(1.0, 1.0, 54.0);
         blandar_sunduk();
         }
         //

         //Dudur
         glPushMatrix();
         glTranslatef(-8.5, 36, -17.5);
         //glScaled(1.5, 28.0, 1.5);
         glRotatef(50.0, 59.0, 0.0, -47.0);
         soko();

         glPushMatrix();
         glTranslatef(-8.5, 36, -6.5);
         //glScaled(1.5, 28.0, 1.5);
         glRotatef(50.0, -59.0, 0.0, -47.0);
         soko();

         glPushMatrix();
         glTranslatef(8.5, 36, -6.5);
         //glScaled(1.5, 28.0, 1.5);
         glRotatef(50.0, -59.0, 0.0, 47.0);
         soko();

         glPushMatrix();
         glTranslatef(8.5, 36, -17.5);
         //glScaled(1.5, 28.0, 1.5);
         glRotatef(50.0, 59.0, 0.0, 47.0);
         glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
         soko();
         //

         //Dudur Luar
         //--sudut
         glPushMatrix();
         glTranslatef(-26, 28, -33.5);
         glRotatef(80.0, 40.0, 0.0, -47.0);
         soko_luar();

         glPushMatrix();
         glTranslatef(-26, 28, 10.0);
         glRotatef(80.0, -40.0, 0.0, -47.0);
         soko_luar();

         glPushMatrix();
         glTranslatef(26, 28, 10.0);
         glRotatef(80.0, -40.0, 0.0, 47.0);
         soko_luar();

         glPushMatrix();
         glTranslatef(26, 28, -33.5);
         glRotatef(80.0, 40.0, 0.0, 47.0);
         soko_luar();

         //--dalam
         for (int i=0; i <= 1; i++ ){
             int j=20;
             glPushMatrix();
             glTranslatef(-26.0, 28.0, -22.0+j*i);
             glRotatef(-77.0, 0.0, 0.0, 40.0);
             soko_dalam();
             }

         for (int i=0; i <= 1; i++ ){
             int j=20;
             glPushMatrix();
             glTranslated(26, 28, -22+j*i);
             glRotatef(77.0, 0.0, 0.0, 47.0);
             soko_dalam();
             }

         //depan
         for (int i=0; i <= 1; i++ ){
             int j=24;
             glPushMatrix();
             glTranslated(-12+j*i, 28, 10);
             glRotatef(75.0, -47.0, 0.0, 0.0);
             soko_dalamdepan();
             }
         //belakang
         for (int i=0; i <= 1; i++ ){
             int j=24;
             glPushMatrix();
             glTranslated(-12+j*i, 28, -34);
             glRotatef(75.0, 47.0, 0.0, 0.0);
             soko_dalamdepan();
             }
         //
        //===============================================//

         //Atap bawah
         //===============================================//
         	 glPushMatrix();
         	 glTranslatef(8.0, 20.0, -35.5);
         	 atap();
         //=================================================//
         	//Atap atas
         	//===============================================//
         	  glPushMatrix();
         	  glTranslatef(8.0, 20.0, -35.5);
         	  atap_atas();
         	//=================================================//

    //Kandang Ayam//

         	glPushMatrix();
         	glTranslatef(58.0, 0.3, -30.0);
         	kandang_ayam();

			//depan_kandang
         	for (int i=0; i <= 8; i++ ){
         	float j=1.0;
         	glPushMatrix();
         	glTranslatef(54.0+j*i, 6.0, -26.0);
         	tiang_kandang();
         	}
         	//kanan_kandang
			for (int i=0; i <= 8; i++ ){
			float j=-1.0;
			glPushMatrix();
			glTranslatef(62.0, 6.0, -26.0+j*i);
			tiang_kandang();
			}
			//kiri_kandang
			for (int i=0; i <= 8; i++ ){
			float j=-1.0;
			glPushMatrix();
		    glTranslatef(54.0, 6.0, -26.0+j*i);
			tiang_kandang();
			}
			//depan_kandang
			for (int i=0; i <= 8; i++ ){
			float j=1.0;
			glPushMatrix();
			glTranslatef(54.0+j*i, 6.0, -34.0);
			tiang_kandang();
			}

			//atap_kandang;
			glPushMatrix();
			glTranslatef(55.0, 15.0, -30.0);
			glRotatef(45,0,0,45);
			atap_kandang();

			glPushMatrix();
			glTranslatef(60.5, 15.0, -30.0);
			glRotatef(45,0,0,-45);
			atap_kandang();

	//pohon
    //--------------------------------------------------------//
			//batang
			glPushMatrix();
			glTranslatef(64.0,0.5,-1.5);
			glScalef(0.5, 1.0, 0.5);
			glRotatef(90,0,1,0);
			pohon();

			//ranting1
			glPushMatrix();
			glTranslatef(64.0,-1.1,-1.5);
			ranting();
			//daun1
			glPushMatrix();
			glTranslatef(64.0,-1.1,-1.5);
			daun();

			//ranting2
			glPushMatrix();
			glTranslatef(64.0,-3.0,-1.5);
			glRotatef(90.0,0.0,60.0,0.0);
			ranting();

			//daun2
			glPushMatrix();
			glTranslatef(58.0,-2.0,-0.5);
			glRotatef(90.0,0.0,60.0,0.0);
			daun();

			//ranting3
			glPushMatrix();
			glTranslatef(64.0,-5.0,-1.5);
			glRotatef(180.0,0.0,-50.0,0.0);
			ranting();

			//daun3
			glPushMatrix();
			glTranslatef(64.0,-2.0,-1.5);
			glRotatef(180.0,0.0,-50.0,0.0);
			daun();

			//daun4
			glPushMatrix();
			glTranslatef(62.0,1.3,15.5);
			glRotatef(180.0,0.0,-90.0,0.0);
			daun();

	//------------------------------------------------------//

	//Jalan Setapak
	//--------------------------------------------------------//
		for (int i=0; i <= 14; i++ ){
		float j=4.5;
		glPushMatrix();
		glTranslatef(-41.0+j*i, -0.3, 39.0);
		//glRotated(20,0.0,20,0);
		//jalan_setapak();
		}

		for (int i=0; i <= 2; i++ ){
		float j=4.5;
		glPushMatrix();
		glTranslatef(-2.0, -0.3, 24.0+j*i);
		glRotated(90,0.0,20,0);
		//jalan_setapak();
		}
	//--------------------------------------------------------//
	//

	//Pagar Rumah//
	//---------------------------------------------------------//
		//depan_kiri
		for (int i=0; i <= 11; i++ ){
		    int j=2;
		    glPushMatrix();
		    glTranslated(-36.0+j*i, 4, 18);
		    glScaled(1.4, 1.8, 0.7);
		    pagar_rumah();
		    }
		//atasnya
			glPushMatrix();
			glTranslated(-24.0, 8, 18);
			glScaled(11.4, 0.6, 0.6);
			pagar_rumah_atas();

		//
		//depan_kanan
		for (int i=0; i <= 10; i++ ){
			int j=2;
			glPushMatrix();
			glTranslated(14.0+j*i, 4, 18);
			glScaled(1.4, 1.8, 0.7);
			pagar_rumah();
			}
		//atasnya
			glPushMatrix();
			glTranslated(24.0, 8, 18);
			glScaled(11.4, 0.6, 0.6);
			pagar_rumah_atas();
				//
		//kiri
		    for (int i=0; i <= 30; i++ ){
		    int j=2;
		    glPushMatrix();
		    glTranslated(-36, 4, -42+j*i);
		    glScaled(0.7, 1.8, 1.4);
		    pagar_rumah();
		    }
		    //atasnya
		    glPushMatrix();
		    glTranslated(-36, 8, -12);
		    glScaled(0.6, 0.6, 30.4);
		    pagar_rumah_atas();
		    //
		//kanan
		    for (int i=0; i <= 30; i++ ){
		    int j=2;
		    glPushMatrix();
		    glTranslated(36, 4, -42+j*i);
		    glScaled(0.7, 1.8, 1.4);
		    pagar_rumah();
		    }
		    //atasnya
		    glPushMatrix();
		    glTranslated(36, 8, -12);
		    glScaled(0.6, 0.6, 30.4);
		    pagar_rumah_atas();
		    //
	    //belakang_kiri
		    for (int i=0; i <= 10; i++ ){
		    int j=2;
		    glPushMatrix();
		    glTranslated(-34+j*i, 4, -42);
		    glScaled(1.4, 1.8, 0.9);
		    pagar_rumah();
		    }
		    //atasnya
		    glPushMatrix();
		    glTranslated(-24.0, 8, -42);
		    glScaled(11.4, 0.6, 0.6);
		    pagar_rumah_atas();

		    		//
		//belakang_kanan
		    for (int i=0; i <= 10; i++ ){
		    int j=2;
		    glPushMatrix();
		    glTranslated(14+j*i, 4, -42);
		    glScaled(1.4, 1.8, 0.9);
		    pagar_rumah();
		    }
		    //atasnya
		    glPushMatrix();
		    glTranslated(24, 8, -42);
		    glScaled(11.4, 0.6, 0.6);
		    pagar_rumah_atas();
		    //

	//---------------------------------------------------------//

	//---------------------------------------------------------//
	//--------------------pagar_luar_kiri
		        glPushMatrix();
		        glTranslated(-45.0, 2, 53);
		        glScaled(20.0, 1.5, 1.0);
		        pagar_luar();
		        glPushMatrix();
		        glTranslatef(-11.5, 4.0, 52.5);
		        glScaled(2.5, 3.0, 2.0);
		        pagar_luar();
    //--------------------pagar_luar_kanan
		        glPushMatrix();
		        glTranslated(45.0, 2, 53);
		        glScaled(20.0, 1.5, 1.0);
		        pagar_luar();
		        glPushMatrix();
		        glTranslatef(11.5, 4.0, 52.5);
		        glScaled(2.5, 3.0, 2.0);
		        pagar_luar();

		  //-------------------------------------------------------------//
		  //Pagar gapura//
		        //depan_kanan
		        //1
		        for (int i=0; i <=15; i++ ){
		        	float j=0.7;
		        	glPushMatrix();
		        	glTranslatef(26.0, 0.7+j*i, 53.0);
		        	atas_gapura();
		        	}
		        	 //ujung gapura
		        	for (int i=0; i <= 3; i++ ){
		        	int j=16;
		        	glPushMatrix();
		        	glTranslatef(26+j*i, 7.35, 53.0);
		        	atas_gapura2();
		        	}
		        //2 tiang_gapura
		        	for (int i=0; i <=15; i++ ){
		        	float j=0.7;
		        	glPushMatrix();
		        	glTranslatef(26+16.0, 0.7+j*i, 53.0);
		        	atas_gapura();
		        	}
		        //3
		        	for (int i=0; i <=15; i++ ){
		        	float j=0.7;
		        	glPushMatrix();
		        	glTranslatef(26+32.0, 0.7+j*i, 53.0);
		        	atas_gapura();
		        	}
		        //4
		        	for (int i=0; i <=15; i++ ){
		        	float j=0.7;
		        	glPushMatrix();
		        	glTranslatef(26+48.0, 0.7+j*i, 53.0);
		        	atas_gapura();
		        	}
		        		//tengan
		        			glPushMatrix();
		        			glTranslated(45.0, 5.6, 53.0);
		        			glScaled(60.4, 0.3, 0.3);
		        			pagar_gapura_atas();
		                //pembatas
		                    glPushMatrix();
		        			glTranslated(62.0, 3.3, 53.0);
		        			glScaled(25.4, 0.3, 0.3);
		        			pagar_gapura_atas();
			                //atas
		                    glPushMatrix();
		        			glTranslated(45.0, 6.6, 53.0);
		        			glScaled(60.4, 0.3, 0.3);
		        			pagar_gapura_atas();

		        			//pagar besi
		        		for (int i=0; i <= 30; i++ ){
		        		    int j=2;
		        		    glPushMatrix();
		        		    glTranslated(15.0+j*i, 6.0, 53.0);
		        		    glScaled(0.7, 2.0, 0.7);
		        		    pagar_gapura();
		        		    }
		        	//---------------------------------------------------------//

		            //---------------------------------------------------------//
		            //lampu 1
		        		for (int i=0; i <=3; i++ ){
		        			//float j=-16;
		                    glPushMatrix();
		        			glTranslated(74.0, 12.7, 53.0);
		        			glScaled(0.4, 0.5, 0.5);
		        			lampu();}
		        	//---------------------------------------------------------//
		        	//tiang lampu 1
		        		for (int i=0; i <=3; i++ ){
		        			float j=-16;
		                    glPushMatrix();
		        			glTranslated(74.0+j*i, 11.7, 53.0);
		        			glScaled(0.4, 1.5, 0.8);
		        			tiang_lampu();}
		            //--------------------------------//
		//ending pagar luar kiri
		        		//depan_kiri
		        				        //1
		        				        for (int i=0; i <=15; i++ ){
		        				        	float j=0.7;
		        				        	glPushMatrix();
		        				        	glTranslatef(-74.0, 0.7+j*i, 53.0);
		        				        	atas_gapura();
		        				        	}
		        				        	 //ujung gapura
		        				        	for (int i=0; i <= 4; i++ ){
		        				        	int j=16;
		        				        	glPushMatrix();
		        				        	glTranslatef(-74+j*i, 7.35, 53.0);
		        				        	atas_gapura2();
		        				        	}
		        				        //2 tiang_gapura
		        				        	for (int i=0; i <=15; i++ ){
		        				        	float j=0.7;
		        				        	glPushMatrix();
		        				        	glTranslatef(-74+16.0, 0.7+j*i, 53.0);
		        				        	atas_gapura();
		        				        	}
		        				        //3
		        				        	for (int i=0; i <=15; i++ ){
		        				        	float j=0.7;
		        				        	glPushMatrix();
		        				        	glTranslatef(-74+32.0, 0.7+j*i, 53.0);
		        				        	atas_gapura();
		        				        	}
		        				        //4
		        				        	for (int i=0; i <=15; i++ ){
		        				        	float j=0.7;
		        				        	glPushMatrix();
		        				        	glTranslatef(-74+48.0, 0.7+j*i, 53.0);
		        				        	atas_gapura();
		        				        	}
		        				        		//tengan
		        				        			glPushMatrix();
		        				        			glTranslated(-45.0, 5.6, 53.0);
		        				        			glScaled(60.4, 0.3, 0.3);
		        				        			pagar_gapura_atas();
		        				                //pembatas
		        				                    glPushMatrix();
		        				        			glTranslated(-62.0, 3.3, 53.0);
		        				        			glScaled(25.4, 0.3, 0.3);
		        				        			pagar_gapura_atas();
		        					                //atas
		        				                    glPushMatrix();
		        				        			glTranslated(-45.0, 6.6, 53.0);
		        				        			glScaled(60.4, 0.3, 0.3);
		        				        			pagar_gapura_atas();

		        				        			//pagar besi
		        				        		for (int i=0; i <= 30; i++ ){
		        				        		    int j=2;
		        				        		    glPushMatrix();
		        				        		    glTranslated(-74.0+j*i, 6.0, 53.0);
		        				        		    glScaled(0.7, 2.0, 0.7);
		        				        		    pagar_gapura();
		        				        		    }
		        				        	//---------------------------------------------------------//

		        				            //---------------------------------------------------------//
		        				            //lampu 1
		        				        		for (int i=0; i <=3; i++ ){
		        				        			//float j=16;
		        				                    glPushMatrix();
		        				        			glTranslated(-74.0, 12.7, 53.0);
		        				        			glScaled(0.4, 0.5, 0.5);
		        				        			lampu();}
		        				        	//---------------------------------------------------------//
		        				        	//tiang lampu 1
		        				        		for (int i=0; i <=3; i++ ){
		        				        			float j=16;
		        				                    glPushMatrix();
		        				        			glTranslated(-74.0+j*i, 11.7, 53.0);
		        				        			glScaled(0.4, 1.5, 0.8);
		        				        			tiang_lampu();}
		        				            //--------------------------------//
		        				//ending gapura luar kiri
		        		//gapura tengah
		        		for (int i=0; i <=15; i++ ){
		        			float j=0.7;
		        			glPushMatrix();
		        			glTranslatef(11.6, 0.7+j*i, 53.0);
		        			atas_gapura();
		        		}

		        		for (int i=0; i <=15; i++ ){
		        		float j=0.7;
		        		glPushMatrix();
		        		glTranslatef(-11.6, 0.7+j*i, 53.0);
		        		atas_gapura();
		        		}
		        		for (int i=0; i <=1; i++ ){
		        		float y=23.2;
		        	    glPushMatrix();
		        		glTranslated(-11.6+y*i, 12.7, 53.0);
		        		glScaled(0.4, 0.8, 0.5);
		        		lampu();}

		        	    //ending gapura tengah


		                 // kaki kursi
		                 glPushMatrix();
		                 glTranslated(-59, 3.5, 42);
		                 glScaled(1.0, 4.0, 1.0);
		                 kursi_kaki();

		                 glPushMatrix();
		                 glTranslated(-49, 3.5, 42);
		                 glScaled(1.0, 4.0, 1.0);
		                 kursi_kaki();

		                 glPushMatrix();
		                 glTranslated(-59, 3.5, 37);
		                 glScaled(1.0, 4.0, 1.0);
		                 kursi_kaki();

		                 glPushMatrix();
		                 glTranslated(-49, 3.5, 37);
		                 glScaled(1.0, 4.0, 1.0);
		                 kursi_kaki();

		                 //atas kursi
		                 glPushMatrix();
		                 glTranslated(-54, 5, 39.5);
		                 glScaled(12, 1.0, 8);
		                 kursi_atas();

		                 //sandaran
		                 glPushMatrix();
		                 glTranslated(-54, 7, 37);
		                 glScaled(12, 4.0, 1);
		                 sandaran();

		                 //tiang lampu taman
		        			glPushMatrix();
		        			glTranslatef(65,0.0,20);
		        			glScalef(0.5, 1.0, 0.5);
		        			glRotatef(90,0,1,0);
		        			tiang();
		                //tutup tiang lampu taman
		        			glPushMatrix();
		        			glTranslatef(65,11.7, 20);
		        			glScalef(0.5, 1.0, 0.5);
		        			glRotatef(90,0,1,0);
		        			tutup_tiang();

		        			//lampu taman
		                    glPushMatrix();
		        			glTranslated(65,13.5, 20);
		        			glScaled(0.4, 0.5, 0.5);
		        			lampu();

		                    //dasar lampu taman
		                    glPushMatrix();
		        			glTranslated(65,0.0, 20);
		        			glScaled(0.7, 1, 0.7);
		        			bawah_gapura1();

		        			//dasar atas lampu taman
		                    glPushMatrix();
		        			glTranslated(65,1, 20);
		        			glScaled(0.5, 1, 0.5);
		        			bawah_gapura1();

		        			//tiang lampu taman 2
		        			glPushMatrix();
		        			glTranslatef(-45,0.0,20);
		        			glScalef(0.5, 1.0, 0.5);
		        			glRotatef(90,0,1,0);
		        			tiang();

		                    //tutup tiang lampu taman 2
		        			glPushMatrix();
		        			glTranslatef(-45,11.7, 20);
		        			glScalef(0.5, 1.0, 0.5);
		        			glRotatef(90,0,1,0);
		        			tutup_tiang();

		        			//lampu taman 2
		                    glPushMatrix();
		        			glTranslated(-45,13.5, 20);
		        			glScaled(0.4, 0.5, 0.5);
		        			lampu();

		                    //dasar lampu taman 2
		                    glPushMatrix();
		        			glTranslated(-45,0.0, 20);
		        			glScaled(0.7, 1, 0.7);
		        			bawah_gapura1();

		        			//dasar atas lampu taman 2
		                    glPushMatrix();
		        			glTranslated(-45,1, 20);
		        			glScaled(0.5, 1, 0.5);
		        			bawah_gapura1();

		        			//batang pohon bulat
		        			glPushMatrix();
		        			glTranslatef(-66.5,0,44.5);
		        			glScalef(0.5, 1.0, 0.5);
		        			glRotatef(90,0,1,0);
		        			pohon();

		        			//ranting1
		        			glPushMatrix();
		        			glTranslatef(-66.5,-1.1,44.5);
		        			ranting();

		        			//daun1
		        			glPushMatrix();
		        			glTranslatef(-66.5,-1.1,44.5);
		        			daun();


		        			//ranting2
		        			glPushMatrix();
		        			glTranslatef(-66.5,-3.0,44.5);
		        			glRotatef(90.0,0.0,60.0,0.0);
		        			ranting();

		        			//daun2
		        			glPushMatrix();
		        			glTranslatef(-70.0,-2.0,44.5);
		        			glRotatef(90.0,0.0,60.0,0.0);
		        			daun();

		        			//ranting3
		        			glPushMatrix();
		        			glTranslatef(-66.5,-8.0,44.5);
		        			glRotatef(180.0,0.0,-50.0,0.0);
		        			ranting();

		        			//daun3
		        			glPushMatrix();
		        			glTranslatef(-67,-9.0,48);
		        			glRotatef(180.0,0.0,-50.0,0.0);
		        			daun();

		        			//daun4
		        			glPushMatrix();
		        			glTranslatef(-68.0,1.5,58.5);
		        			glRotatef(180.0,0.0,-90.0,0.0);
		        			daun();

		        			//ranting5
		        			glPushMatrix();
		        			glTranslatef(-66.5,-8.0,44.5);
		        			glRotatef(-270.0,0.0,-50.0,0.0);
		        			ranting();

		        			//daun5
		        			glPushMatrix();
		        			glTranslatef(-64.0,-10,44.5);
		        			glRotatef(-270.0,0.0,-50.0,0.0);
		        			daun();

		        			//ending pohon//
	//tiang kolam------------------//
	glPushMatrix();
	glTranslatef(45.8,-1.0,40.0);
	tiang_kolam();

	glPushMatrix();
	glTranslatef(45.8,5.0,40.0);
	glScalef(4.0,1.5,4.0);
	atas_gapura();
	//ending tiang kolam------------//

	//Jalan setapak-------------------//

	for (int i=0; i <= 14; i++ ){
	float j=4.5;
	glPushMatrix();
	glTranslatef(-58.0+j*i, -0.3, -51.0);
	//glRotated(20,0.0,20,0);
	jalan_setapak();
	}
	for (int i=0; i <= 20; i++ ){
	float j=4.5;
	glPushMatrix();
	glTranslatef(-64.0, -0.3, -50.0+j*i);
	glRotated(90,0.0,20,0);
	jalan_setapak();
	}
	//----------------------------------//

	//----------------------------------//
	glPushMatrix();
	glTranslatef(30, 90, -150);
	glScalef(1.8, 1.0, 1.0);
	awan();
	//---------------------------------//
	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}


static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;

	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;

	/*case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;*/
	default:
		break;
	}
}

void newkeyboard(unsigned char key, int x, int y) {
	/*if (key == 'd') {

		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}*/
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
//UNTUK BUS
    if  (key == 'm'){
        a+=-1;
    d+=1;
    b=0;
    c=0;
      }
      else if (key=='n'){
           d+=-1;
  a+=1;
  b=0;
  c=0;
    }
    if  (key == 27){ //ESC
        exit(0);
    }
}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Tugas Besar Grafkom- Rumah Joglo");
	init();
	initRendering();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);

	glutKeyboardFunc(newkeyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	glutMainLoop();
	return 0;
}
