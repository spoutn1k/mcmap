/*
 *  extractcolors.h
 *  MCMap Live
 *
 *  Created by DK on 10/18/10.
 *
 */


// This beast of an array maps block IDs to tile locations in terrain.png
// The tile x and tile y are 0-index. A value of -1,-1 means no tile exists
// Extra alpha multiplier is there for textures that are shared and might need to
// be lighter for one use than another.
//                                  { tile x, tile y, extra alpha multiplier)
const int special_sauce[86][3] = {  {	-1,	-1,	1	},	
                                    {	1,	0,	1	},	
                                    {	0,	0,	1	},	
                                    {	2,	0,	1	},	
                                    {	0,	1,	1	},	
                                    {	4,	0,	1	},	
                                    {	15,	0,	1	},	
                                    {	1,	1,	1	},	
                                    {	15,	12,	1	},	
                                    {	15,	12,	1	},	
                                    {	15,	14,	1	},	
                                    {	15,	14,	1	},	
                                    {	2,	1,	1	},	
                                    {	3,	1,	1	},	
                                    {	0,	2,	1	},	
                                    {	1,	2,	1	},	
                                    {	2,	2,	1	},	
                                    {	4,	1,	1	},	
                                    {	5,	3,	1	},	
                                    {	0,	3,	1	},	
                                    {	1,	3,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	0,	4,	1	},	
                                    {	-1,	-1,	1	},	
                                    {	13,	0,	1	},	
                                    {	12,	0,	1	},	
                                    {	13,	1,	1	},	
                                    {	12,	1,	1	},	
                                    {	7,	2,	1	},	
                                    {	6,	2,	1	},	
                                    {	5,	0,	1	},	
                                    {	5,	0,	1	},	
                                    {	7,	0,	1	},	
                                    {	8,	0,	1	},	
                                    {	3,	2,	1	},	
                                    {	4,	2,	1	},	
                                    {	5,	2,	1	},	
                                    {	0,	5,	1	},	
                                    {	15,	15,	0.3	},	
                                    {	1,	4,	1	},	
                                    {	4,	0,	1	},	
                                    {	11,	1,	1	},	
                                    {	4,	6,	1	},	
                                    {	2,	3,	1	},	
                                    {	8,	2,	1	},	
                                    {	12,	3,	1	},	
                                    {	15,	5,	1	},	
                                    {	7,	5,	1	},	
                                    {	12,	2,	1	},	
                                    {	13,	3,	1	},	
                                    {	0,	0,	1	},	
                                    {	1,	6,	1	},	
                                    {	3,	5,	1	},	
                                    {	0,	8,	1	},	
                                    {	0,	1,	1	},	
                                    {	4,	0,	1	},	
                                    {	3,	6,	1	},	
                                    {	0,	6,	1	},	
                                    {	2,	6,	1	},	
                                    {	4,	0,	1	},	
                                    {	3,	3,	1	},	
                                    {	3,	3,	1	},	
                                    {	3,	7,	1	},	
                                    {	3,	6,	1	},	
                                    {	2,	6,	0.1	},	
                                    {	2,	4,	1	},	
                                    {	3,	4,	1	},	
                                    {	2,	4,	1	},	
                                    {	6,	4,	1	},	
                                    {	8,	4,	1	},	
                                    {	9,	4,	1	},	
                                    {	10,	4,	1	},	
                                    {	5,	1,	0.6	}	};
                                    
void getTileRGBA(unsigned char *textures, int tilesize, int x, int y, int &r, int &g, int &b, int &a, int &noise)
{
    r = 0;
    g = 0;
    b = 0;
    a = 0;
    noise = 0;
    
    if (x == -1)
        return;
    
    int n = tilesize*tilesize;
    int bytesperrow = 16*tilesize*4;
    
    int sx = x*tilesize*4;
    int sy = y*tilesize;
    
    for(int j=sy; j<(sy+tilesize); j++)
    {
        for(int i=sx; i<(sx+tilesize*4); i=i+4)
        {
            // If the pixel is entirely transparent
            if (textures[i+3+j*(bytesperrow)] == 0)
                n--;
            else
            {
                r=r+textures[i+j*(bytesperrow)];
                g=g+textures[i+1+j*(bytesperrow)];
                b=b+textures[i+2+j*(bytesperrow)];
                a=a+textures[i+3+j*(bytesperrow)];
            }
            
        }
    }
    
    double var = 0;
    
    if (n>0)
    {
        r=r/n;
        g=g/n;
        b=b/n;
        a=a/n;
    
        for(int j=sy; j<(sy+tilesize); j++)
        {
            for(int i=sx; i<(sx+tilesize*4); i=i+4)
            {
                // If the pixel is entirely transparent
                if (textures[i+3+j*(bytesperrow)] != 0)
                {
                    var = var + (pow(textures[i+j*(bytesperrow)]-r,2) + pow(textures[i+1+j*(bytesperrow)]-b,2) + pow(textures[i+2+j*(bytesperrow)]-g,2))/(3*n);
                }
                
            }
        }
            
        noise = int(8 * var / ((n*n-1)/12) );
        if (noise > 255)
            noise = 255;
    }
    
}

void extractcolors(unsigned char *textures, int tilesize, int (*colors)[5] )
{
    for(int i=0; i<86; i++)
    {
        getTileRGBA(textures, tilesize, special_sauce[i][0],special_sauce[i][1],colors[i][0],colors[i][1],colors[i][2],colors[i][3],colors[i][4]);
        colors[i][3] = colors[i][3]*special_sauce[i][2];
    }
}

