/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Tiziano <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "SMFMap.h"
#include <cstring>
#include <iostream>
#include <stdlib.h>
SMFMap::SMFMap(std::string name,std::string texturepath)
{
    m_tiles = new TileStorage();
    metalmap = NULL;
    heightmap = NULL;
    typemap = NULL;
    minimap = NULL;
    texture = new Image(texturepath.c_str());
    
    if ( texture->w < 1 )
        throw CannotLoadTextureException();
    if ( texture->w % 1024 != 0 || texture->h % 1024 != 0)
    {
        throw InvalidMapSizeException();

    }
    mapx = ( texture->w / 1024 ) * 128;
    mapy = ( texture->w / 1024 ) * 128;
    m_minh = 0.0;
    m_maxh = 1.0;
    m_name = name;
    m_th = 0;
    m_comptype = COMPRESS_REASONABLE;
    texpath = texturepath;
}
void SMFMap::SetHeightRange(float minh, float maxh)
{
  m_minh = minh;
  m_maxh = maxh;
}

SMFMap::~SMFMap()
{
    delete m_tiles;
    if ( metalmap )
      delete metalmap;
    if ( heightmap )
      delete heightmap;
    if ( typemap )
      delete typemap;
    if ( minimap )
      delete minimap;
    if ( texture )
      delete texture;
}
void SMFMap::SetMiniMap(std::string path)
{
    delete texture;
    std::cout << "Loading minimap " << path << std::endl;
    Image * img = new Image(path.c_str());
    if ( img->w > 0 )
    {
        if ( minimap )
            delete minimap;
        minimap = img;
        minimap->ConvertToRGBA();
	minimap->FlipVertical();
	if ( img->w != 1024 || img->h != 1024 )
      {
	  std::cerr << "Warning: Minimap has wrong size , rescaling!" << std::endl;
	  minimap->Rescale(1024,1024);

      }
      texture = new Image(texpath.c_str());
      return;
    }
    std::cout << "Failed " << path << std::endl;
    texture = new Image(texpath.c_str());
}

/*void SMFMap::SetFeatureMap(std::string path)
{
  Image * img = new Image(path.c_str());
  if ( img->w > 0 )
  {
    if ( featuremap )
      delete featuremap;
    featuremap = img;
    featuremap->ConvertToLUM();
  }
  if ( img->w != mapx/2 || img->h != mapy/2 )
  {
    std::cerr << "Warning: Feature map has wrong size , rescaling!" << std::endl;
    heightmap->Rescale(mapx+1,mapy+1);

  }
}*/
void SMFMap::SetHeightMap(std::string path)
{
    Image * img = new Image(path.c_str());
    if ( img->w > 0 )
    {
        if ( heightmap )
            delete heightmap;
        heightmap = img;
        heightmap->ConvertToLUM();
        if ( img->w != mapx+1 || img->h != mapy+1 )
        {
            std::cerr << "Warning: Height map has wrong size , rescaling!" << std::endl;
            heightmap->Rescale(mapx+1,mapy+1);

        }
    }

}
void SMFMap::SetMetalMap(std::string path)
{
    Image * img = new Image(path.c_str());
    if ( img->w > 0 )
    {
        if ( metalmap )
            delete metalmap;
        metalmap = img;
        metalmap->ConvertToLUM();
        if ( img->w != mapx/2 || img->h != mapy/2 )
        {
            std::cerr << "Warning: Metal map has wrong size , rescaling!" << std::endl;
            metalmap->Rescale( mapx/2,mapy/2);

        }
    }
}
void SMFMap::SetTypeMap(std::string path)
{
    Image * img = new Image(path.c_str());
    if ( img->w > 0 )
    {
        if ( typemap )
            delete typemap;
        typemap = img;
        typemap->ConvertToLUM();
        if ( img->w != mapx/2 || img->h != mapy/2 )
        {
            std::cerr << "Warning: Type map has wrong size , rescaling!" << std::endl;
            typemap->Rescale(mapx/2,mapy/2);

        }

    }

}

void SMFMap::Compile()
{
    SMFHeader hdr;
    strcpy(hdr.magic,"spring map file");
    hdr.version = 1;
    hdr.mapid = rand();
    hdr.mapx = (texture->w / 1024)*128;
    hdr.mapy = (texture->h / 1024)*128;
    hdr.squareSize = 8;
    hdr.texelPerSquare = 8;
    hdr.tilesize = 32;
    hdr.minHeight = m_minh;
    hdr.maxHeight = m_maxh;


    short int * hmap = new short int[(mapy+1)*(mapx+1)];bzero(hmap,((mapy+1)*(mapx+1))*2);
    if ( heightmap )
    {
        heightmap->GetRect(0,0,heightmap->w,heightmap->h,IL_LUMINANCE,IL_SHORT,hmap);
    }
    unsigned char * typedata = new unsigned char[mapy/2 * mapx/2];bzero(typedata,(mapy/2 * mapx/2));
    if ( typemap )
    {
      typemap->GetRect(0,0,typemap->w,typemap->h,IL_LUMINANCE,IL_UNSIGNED_BYTE,typedata);
      
    }
    uint8_t * minimap_data = new uint8_t[699064];
    bzero(minimap_data,699064);
    if ( minimap )
    {
        int p = 0;
        int s = 1024;

        Image * im2 = new Image();
        im2->AllocateRGBA(1024,1024,(char*)minimap->datapointer);
        for ( int i = 0; i < 9; i++ )
        {
	    std::cout << ">Mipmap " << i << std::endl;
	    im2->Rescale(s,s);
	    std::cout << "<Mipmap " << i << std::endl;
	    ILuint ss;
            ILubyte * dxtdata = ilCompressDXT(im2->datapointer,s,s,1,IL_DXT1,&ss);
	    std::cout << ss << " " << s;
	    memcpy(&minimap_data[p],dxtdata,ss);
	    free(dxtdata);
            p += ss;
	    
            s = s >> 1;
	    
        }
        delete im2;

    }
    unsigned char * metalmap_data = new unsigned char[mapx/2 * mapy/2];bzero(metalmap_data,(mapy/2 * mapx/2));
    if ( metalmap )
    {
      metalmap->GetRect(0,0,metalmap->w,metalmap->h,IL_LUMINANCE,IL_UNSIGNED_BYTE,metalmap_data);
      
    }
    /*hdr.heightmapPtr = sizeof(hdr);
    hdr.typeMapPtr = hdr.heightmapPtr + ((mapy+1)*(mapx+1))*2;
    hdr.minimapPtr = hdr.typeMapPtr + (mapy/2 * mapx/2);
    hdr.metalmapPtr = hdr.minimapPtr + 699048;
    hdr.featurePtr = hdr.metalmapPtr + (mapy/2 * mapx/2);*/
    MapFeatureHeader mfhdr;
    mfhdr.numFeatures = 0;
    mfhdr.numFeatureType = 0; // TODO
    hdr.tilesPtr = hdr.featurePtr + sizeof(mfhdr);
    hdr.numExtraHeaders = 0; // TODO : Grass
    MapTileHeader mthdr;
    mthdr.numTileFiles = 1;
    
    
    int * tiles = new int[mapx/4 * mapy/4];
    std::vector<uint64_t> order;
    DoCompress(tiles,order);
   /* for ( int y = 0; y < mapy/4; y++ )
    {
    for ( int x = 0; x < mapx/4; x++ )
      {
	printf("%5d,",tiles[(mapx/4)*y+x]);
      }
      printf("\n");
    }*/
    
    FILE * tilefile = fopen((m_name+std::string(".smt")).c_str(),"wb");
    delete texture; //Temporarily delete texture from memory to reduce mem usage
    m_tiles->WriteToFile(tilefile,order);
    texture = new Image(texpath.c_str());
     
    fclose(tilefile);
    FILE * smffile = fopen((m_name+std::string(".smf")).c_str(),"wb");
    fwrite(&hdr,sizeof(hdr),1,smffile);
    std::cout << ftell(smffile) << std::endl;;
    hdr.minimapPtr  = ftell(smffile);
    fwrite(minimap_data,699064,1,smffile);
    hdr.heightmapPtr = ftell(smffile);
    fwrite(hmap,((mapy+1)*(mapx+1))*2,1,smffile);
    hdr.typeMapPtr = ftell(smffile);
    fwrite(typedata,mapy/2 * mapx/2,1,smffile);
    
    hdr.metalmapPtr  = ftell(smffile);
    fwrite(metalmap_data,mapy/2 * mapx/2,1,smffile);
    hdr.featurePtr  = ftell(smffile);
    fwrite(&mfhdr,sizeof(mfhdr),1,smffile);
    hdr.tilesPtr = ftell(smffile);
    uint32_t tc = m_tiles->GetTileCount();
    mthdr.numTiles = tc;
    fwrite(&mthdr,sizeof(mthdr),1,smffile);
    fwrite(&tc,4,1,smffile);
    fwrite((m_name+std::string(".smt")).c_str(),(m_name+std::string(".smt")).length()+1,1,smffile);
    fwrite(tiles,(mapx/4 * mapy/4)*4,1,smffile);
    fseek(smffile,0,SEEK_SET);
    fwrite(&hdr,sizeof(hdr),1,smffile);
    fclose(smffile);
    delete metalmap_data;
    delete hmap;
    delete typedata;
    delete tiles;
    delete minimap_data;
}

void SMFMap::DoCompress(int* indices, std::vector< uint64_t >& order)
{
  order.clear();
  
  uint8_t tiledata[32*32*4];
  std::map<uint64_t,uint32_t> existingtiles;
  for ( int y = 0; y < mapy/4; y++ )
  {
    for ( int x = 0; x < mapx/4; x++ )
    {
      texture->GetRect(x*32,y*32,32,32,IL_RGBA,IL_UNSIGNED_BYTE,tiledata);
      for ( int yy = 0; yy < 16; yy++ )//Flip vertically
      {
	char tmprow[32*4];
	memcpy(tmprow,&tiledata[(31-yy)*32*4],32*4);
	memcpy(&tiledata[(31-yy)*32*4],&tiledata[yy*32*4],32*4);
	memcpy(&tiledata[yy*32*4],tmprow,32*4);
      }
     // std::cout << "Compressing (" << x << "," << y << ")" << std::endl;
      uint64_t uid = m_tiles->AddTileOrGetSimiliar(tiledata,m_th,m_comptype);
      if ( existingtiles.find(uid) == existingtiles.end() )
      {
	indices[(mapx/4)*y+x] = order.size();
	existingtiles[uid] = order.size();
	order.push_back(uid);
      }else{
	indices[(mapx/4)*y+x] = existingtiles[uid];
	
      }
      
      
    }
    
    
    
  }
  std::cout << "Compress done , ratio: " << float(existingtiles.size())/float(mapy/4 * mapx/4)*100.0 << std::endl;
  
}
void SMFMap::SetCompressionTol(float th)
{
  m_th = th;
}
void SMFMap::SetCompressionType(int c)
{
  m_comptype = c;
}
