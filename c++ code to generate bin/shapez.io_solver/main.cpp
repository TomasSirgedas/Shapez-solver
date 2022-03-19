#include <string>
#include <algorithm>
#include <regex>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <deque>

#include "XY.h"
#include "trace.h"

using namespace std;


class Layer
{
public:
   Layer( uint8_t b=0 ) : b(b) {}
   bool isEmpty() const { return b == 0; }
   Layer rotated() const { return ((b<<1)&15) | ((b&8) ? 1 : 0); }
   Layer flipped() const { return (b&5) | ((b&8) ? 2 : 0) | ((b&2) ? 8 : 0); }
   string str() const { string s; for ( int i = 0; i < 4; i++ ) s += b & (1<<i) ? "Cu" : "--"; return s; }
   Layer cutRight() const { return b & 3; }
   Layer cutLeft() const { return b & 12; }
   bool intersects( const Layer& rhs ) const { return b & rhs.b; }

public:
   uint8_t b;
};

class Shape
{
public:
   int numLayers() const { for ( int i = 3; i >= 0; i-- ) if ( !layers[i].isEmpty() ) return i+1; return 0; }
   string str() const { string s; for ( int i = 0; i < numLayers(); i++ ) { if (i) s += ":"; s += layers[i].str(); } return s; }
   Shape rotated() const { Shape ret; for ( int i = 0; i < 4; i++ ) ret.layers[i] = layers[i].rotated(); return ret; }
   Shape flipped() const { Shape ret; for ( int i = 0; i < 4; i++ ) ret.layers[i] = layers[i].flipped(); return ret; }
   Shape withEmptyLayersCollapsed() const { Shape ret; int k = 0; for ( int i = 0; i < 4; i++ ) { ret.layers[k] = layers[i]; k += !ret.layers[k].isEmpty(); } return ret; }
   Shape cutRight() const { Shape ret; for ( int i = 0; i < 4; i++ ) ret.layers[i] = layers[i].cutRight(); return ret.withEmptyLayersCollapsed(); }
   Shape cutLeft() const { Shape ret; for ( int i = 0; i < 4; i++ ) ret.layers[i] = layers[i].cutLeft(); return ret.withEmptyLayersCollapsed(); }
   bool intersects( const Shape& b, int bLayerOffset ) const { for ( int i = bLayerOffset; i < 4; i++ ) if ( layers[i].intersects( b.layers[i-bLayerOffset] ) ) return true; return false; }
   uint16_t code() const { return layers[0].b | (layers[1].b << 4) | (layers[2].b << 8) | (layers[3].b << 12); }
   static Shape fromCode( uint16_t q ) { Shape ret; ret.layers[0].b = q&15; ret.layers[1].b = (q>>4)&15; ret.layers[2].b = (q>>8)&15; ret.layers[3].b = (q>>12)&15; return ret; }
   bool hasFloatingLayer() const { for ( int i = 0; i < 3; i++ ) if ( layers[i+1].b && (layers[i+1].b & layers[i].b) == 0 ) return true; return false; }
   Shape orientated( int k ) const
   {
      Shape ret = *this;
      if ( k&4 ) ret = ret.flipped();
      if ( k&2 ) ret = ret.rotated().rotated();
      if ( k&1 ) ret = ret.rotated();
      return ret;
   }
   Shape canonicalized() const
   {
      uint16_t ret = code();
      for ( int i = 1; i < 8; i++ )
         ret = std::min( ret, orientated( i ).code() );
      return Shape::fromCode( ret );
   }
   bool isCanonical() const
   {
      return code() == canonicalized().code();
   }

public:
   Layer layers[4];
};

int bLayerOffsetForStacking( const Shape& a, const Shape& b )
{
   int bLayerOffset;
   for ( bLayerOffset = a.numLayers() - 1; bLayerOffset >= 0; bLayerOffset-- )
      if ( a.intersects( b, bLayerOffset ) )
      {
         bLayerOffset++;
         break;
      }
   return std::max( bLayerOffset, 0 );
}

// stack b onto a
Shape stack( const Shape& a, const Shape& b )
{
   Shape ret;

   int bLayerOffset = bLayerOffsetForStacking( a, b );

   for ( int i = 0; i < bLayerOffset; i++ )
      ret.layers[i] = a.layers[i];
   for ( int i = bLayerOffset; i < 4; i++ )
      ret.layers[i] = a.layers[i].b | b.layers[i-bLayerOffset].b;

   return ret;
}

class Rect
{
public:
   Rect( XY pt0, XY pt1 ) : _Pt0(pt0), _Pt1(pt1) {}
   XY size() { return _Pt1 - _Pt0; }
   Rect operator|( const Rect& rhs ) const 
   { 
      return Rect( XY( std::min( _Pt0.x, rhs._Pt0.x ), std::min( _Pt0.y, rhs._Pt0.y ) )
                   , XY( std::max( _Pt1.x, rhs._Pt1.x ), std::max( _Pt1.y, rhs._Pt1.y ) ) );
   }

public:
   XY _Pt0;
   XY _Pt1;
};

enum Op : uint8_t { NONE=0, RAW=1, STACK=2, CUT_LEFT=3, CUT_RIGHT=4, ROTATE_1=5, ROTATE_2=6, ROTATE_3=7 };

struct Mapping // where does each bit end up
{
   int m[16];

   Mapping() 
   {
      for ( int i = 0; i < 16; i++ )
         m[i] = -1;
   }

   static Mapping identity() {
      Mapping ret;
      for ( int i = 0; i < 16; i++ )
         ret.m[i] = i;
      return ret;
   }

   int operator[]( int idx ) const { return idx >= 0 && idx < 16 ? m[idx] : -1; }

   Mapping operator*( const Mapping& rhs ) const
   {
      Mapping ret;
      for ( int i = 0; i < 16; i++ )
         ret.m[i] = (*this)[rhs[i]];
      return ret;
   }
};

#pragma pack(push, 1)
struct Recipe
{
   uint16_t a = 0;
   uint16_t b = 0;
   Op op = NONE;

   Mapping mappingForA() const
   {
      if ( op == RAW ) 
      {
         Mapping ret;
         for ( int i = 0; i < 16; i++ )
            ret.m[i] = i;
         return ret;
      }
      if ( op == STACK )
      {
         return Mapping::identity();
      }
      if ( op == CUT_LEFT || op == CUT_RIGHT )
      {
         Shape shape = Shape::fromCode( a );

         int mask = op == CUT_LEFT ? 12 : 3;

         int layerHasSomething[4] = {
            ((shape.layers[0].b & mask) ? 1 : 0),
            ((shape.layers[1].b & mask) ? 1 : 0),
            ((shape.layers[2].b & mask) ? 1 : 0),
            ((shape.layers[3].b & mask) ? 1 : 0)
         };

         int layerMapping[4];
         layerMapping[0] = 0;
         layerMapping[1] = layerHasSomething[0];
         layerMapping[2] = layerHasSomething[0] + layerHasSomething[1];
         layerMapping[3] = layerHasSomething[0] + layerHasSomething[1] + layerHasSomething[2];

         int side = op == CUT_LEFT ? 2 : 0;

         Mapping ret;
         for ( int i = 0; i < 16; i++ )
            ret.m[i] = layerHasSomething[i/4] && ((i & 2) == side) ? layerMapping[i/4]*4 + (i&3) : -1;
         return ret;
      }

      //if ( op == CUT_RIGHT ) return "CUT_RIGHT";


      Mapping rotateMapping;
      for ( int i = 0; i < 16; i++ )
         rotateMapping.m[i] = ((i+1)&3) | (i&12);

      if ( op == ROTATE_1 )
         return rotateMapping;
      if ( op == ROTATE_2 )
         return rotateMapping * rotateMapping;
      if ( op == ROTATE_3 )
         return rotateMapping * rotateMapping * rotateMapping;

      throw 777;
   }

   Mapping mappingForB() const
   {
      int bLayerOffset = bLayerOffsetForStacking( Shape::fromCode( a ), Shape::fromCode( b ) );
      if ( op == STACK )
      {
         Mapping ret;
         for ( int i = 0; i < 16; i++ )
            ret.m[i] = i + bLayerOffset*4;
         return ret;
      }
      throw 777;
   }
};
#pragma pack(pop)

struct ShapeInfo
{
   int bestCost;
   Recipe recipe;
};

//5: compact merger
//6: compact merger
//7: extractor
//8: extractor
//9: cut
//10: 4-way cut
//11: rotate clock
//12: rotate counter-clock
//13: rotate 180
//14: stacker
//15: mixer
//16: painter
//17: painter-flip
//18: painter 2x
//19: painter 4x
//20: trash
//21: storage
//22: tunnel in
//23: tunnel out (long)
//24: tunnel in
//25: tunnel out (long)
//61: item producer

enum BuildingType : int 
{  
   BELT = 1,
   BELT_LEFT = 2,
   BELT_RIGHT = 3,
   CUTTER = 9,
   ROTATOR_1 = 11,
   ROTATOR_2 = 13,
   ROTATOR_3 = 12,
   STACKER = 14,
   TRASH = 20,
   CONSTANT_SIGNAL = 31,
   PRODUCER = 61
};

XY buildingSize( BuildingType type )
{
   if ( type == CUTTER ) return XY(2,1);
   if ( type == STACKER ) return XY(2,1);
   return XY(1,1);
}

class Building
{
public:
   Building( BuildingType type, XY pos, int rotation = 0 ) : _Type(type), _Pos(pos), _Rotation(rotation) {}
   virtual string toJson() const
   {
      string j = R"({"components":{"StaticMapEntity":{"origin":{"x":~X~,"y":~Y~},"rotation":~ROTATION~,"originalRotation":0,"code":~CODE~}~EXTRA_JSON~}})";
      j = std::regex_replace( j, std::regex("~X~"), std::to_string( _Pos.x ) );
      j = std::regex_replace( j, std::regex("~Y~"), std::to_string( _Pos.y ) );
      j = std::regex_replace( j, std::regex("~ROTATION~"), std::to_string( _Rotation*90 ) );
      j = std::regex_replace( j, std::regex("~CODE~"), std::to_string( (int)_Type ) );
      j = std::regex_replace( j, std::regex("~EXTRA_JSON~"), extraJson() );
      return j;
   }
   virtual string extraJson() const
   {
      return "";
   }
   virtual std::shared_ptr<Building> clone( XY offset ) const
   {
      return std::shared_ptr<Building>( new Building( _Type, _Pos + offset, _Rotation ) );
   }
   virtual Rect rect() const 
   { 
      XY sz = buildingSize( _Type );
      if ( _Rotation == 0 ) return Rect( _Pos, _Pos + sz ); 
      if ( _Rotation == 1 ) return Rect( XY( _Pos.x-sz.y+1, _Pos.y ), XY( _Pos.x+1, _Pos.y+sz.x ) ); 
      if ( _Rotation == 2 ) return Rect( _Pos - sz + XY(1,1), _Pos + XY(1,1) ); 
      if ( _Rotation == 3 ) return Rect( XY( _Pos.x, _Pos.y-sz.x+1 ), XY( _Pos.x+sz.y, _Pos.y+1 ) );
      throw 777;
   }

public:
   BuildingType _Type;
   XY _Pos;
   int _Rotation;
};


string codeForShape( const Shape& shape, const string& finalTarget, const Mapping& mapping )
{
   string ret;
   for ( int layer = 0; layer < shape.numLayers(); layer++ )
   {
      if ( layer > 0 )
         ret += ":";
      for ( int b = 0; b < 4; b++ )
      {
         int targetIdx = mapping[layer*4+b];
         if ( !(shape.layers[layer].b & (1<<b)) )
            ret += "--";
         else
            ret += targetIdx >= 0 ? finalTarget.substr( targetIdx*2 + targetIdx/4, 2 ) : "Cu";
      }
   }
   return ret;
}

class ConstantShapeSignal : public Building
{
public:
   ConstantShapeSignal( Shape shape, const string& code, XY pos, int rotation = 0 ) : Building( CONSTANT_SIGNAL, pos, rotation ), _Shape( shape ), _Code( code ) {}
   string extraJson() const override
   {
      string j = R"(,"ConstantSignal":{"signal":{"$":"shape","data":"~SHAPE_CODE~"}})";
      j = std::regex_replace( j, std::regex("~SHAPE_CODE~"), _Code /*_Shape.str()*/ );
      return j;
   }
   std::shared_ptr<Building> clone( XY offset ) const override
   {
      return std::shared_ptr<Building>( new ConstantShapeSignal( _Shape, _Code, _Pos + offset, _Rotation ) );
   }

public:
   Shape _Shape;
   string _Code;
};

class BluePrint
{
public:
   void add( const std::shared_ptr<Building>& building )
   {
      _Buildings.push_back( building );
   }
   void add( const BluePrint& bp, XY offset )
   {
      for ( const std::shared_ptr<Building>& building : bp._Buildings )
         _Buildings.push_back( building->clone( offset ) );
   }
   Rect rect() const
   {
      Rect ret = { XY(9999,9999), XY(-9999,-9999) };
      for ( const std::shared_ptr<Building>& building : _Buildings )
         ret = ret | building->rect();
      return ret;
   }
   string toJson() const
   {
      stringstream ss;
      ss << "[" << endl;
      int first = true;
      for ( const std::shared_ptr<Building>& building : _Buildings )
      {
         ss << (first?"":",") << building->toJson()<< endl;
         first = false;
      }
      ss << "]" << endl;
      return ss.str();
   }

public:
   std::vector<std::shared_ptr<Building>> _Buildings;
};

//ShapeInfo g_shapeInfo[1<<16];
//uint8_t g_shapeIsPossible[1<<16] = { 0 };


string opStr( Op op )
{
   if ( op == RAW ) return "RAW";
   if ( op == STACK ) return "STACK";
   if ( op == CUT_LEFT ) return "CUT_LEFT";
   if ( op == CUT_RIGHT ) return "CUT_RIGHT";
   if ( op == ROTATE_1 ) return "ROTATE_1";
   if ( op == ROTATE_2 ) return "ROTATE_2";
   if ( op == ROTATE_3 ) return "ROTATE_3";
   return "UNKNOWN-OP";
}


Shape shapeFromCode( const std::string& code )
{
   Shape shape;
   for ( int layer = 0; layer < 4; layer++ )
   {
      for ( int b = 0; b < 4; b++ )
      {
         int strIndex = layer*9 + b*2;
         if ( strIndex+2 <= (int)code.length() && code.substr( strIndex, 2 ) != "--" )
            shape.layers[layer].b |= 1 << b;
      }
   }

   return shape;
}


class Recipes
{
public:
   Recipes()
   {
      _Recipes.resize( 1<<16 );
   }
   Recipes( const std::string& filename ) : Recipes()
   {
      loadFromFile( filename );
   }

   const Recipe& operator[]( int index ) const { return _Recipes[index]; }
   Recipe& operator[]( int index ) { return _Recipes[index]; }

   string recipeTreeFor( const Shape& shape, const std::string& prefix )
   {
      stringstream ss;
      const Recipe& recipe = _Recipes[shape.code()];
      ss << prefix << shape.str() << " " << opStr( recipe.op ) <<  endl;
      if ( recipe.a )
         ss << recipeTreeFor( Shape::fromCode( recipe.a ), prefix + "  " );
      if ( recipe.b )
         ss << recipeTreeFor( Shape::fromCode( recipe.b ), prefix + "  " );
      return ss.str(); 
   }

   BluePrint bluePrintFor( const string& finalTarget )
   {
      return bluePrintFor( shapeFromCode( finalTarget ), finalTarget, Mapping::identity() );
   }
   BluePrint bluePrintFor( const Shape& shape, const string& finalTarget, const Mapping& mapping )
   {
      BluePrint ret;

      const Recipe& recipe = _Recipes[shape.code()];
      if ( recipe.op == RAW )
      {
         ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,0), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,1), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,2), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,3), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new Building( PRODUCER, XY(0,4), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new ConstantShapeSignal( shape, codeForShape( shape, finalTarget, mapping ), XY(0,5), 0 ) ) );
      }
      if ( recipe.op == STACK )
      {
         ret.add( std::shared_ptr<Building>( new Building( STACKER, XY(0,0), 0 ) ) );
         BluePrint a = bluePrintFor( Shape::fromCode( recipe.a ), finalTarget, mapping * recipe.mappingForA() );
         BluePrint b = bluePrintFor( Shape::fromCode( recipe.b ), finalTarget, mapping * recipe.mappingForB() );

         int bx = a.rect()._Pt1.x;
         ret.add( a, XY(0,2) );
         ret.add( b, XY(bx,0) + XY(0,2) );
         ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,1), 0 ) ) );

         if ( bx == 1 )
         {
            ret.add( std::shared_ptr<Building>( new Building( BELT, XY(bx,1), 0 ) ) );
         }
         else
         {
            ret.add( std::shared_ptr<Building>( new Building( BELT_RIGHT, XY(1,1), 3 ) ) );
            for ( int x = 2; x < bx; x++ )
               ret.add( std::shared_ptr<Building>( new Building( BELT, XY(x,1), 3 ) ) );
            ret.add( std::shared_ptr<Building>( new Building( BELT_LEFT, XY(bx,1), 0 ) ) );
         }
      }
      if ( recipe.op == CUT_LEFT || recipe.op == CUT_RIGHT )
      {
         ret.add( std::shared_ptr<Building>( new Building( CUTTER, XY(0,2), 0 ) ) );
         ret.add( std::shared_ptr<Building>( new Building( TRASH, recipe.op == CUT_LEFT ? XY(1,1) : XY(0,1), 0 ) ) );
         if ( recipe.op == CUT_LEFT )
         {
            ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,0), 0 ) ) );
            ret.add( std::shared_ptr<Building>( new Building( BELT, XY(0,1), 0 ) ) );
         }
         else
         {
            ret.add( std::shared_ptr<Building>( new Building( BELT_RIGHT, XY(0,0), 3 ) ) );
            ret.add( std::shared_ptr<Building>( new Building( BELT_LEFT, XY(1,0), 0 ) ) );
            ret.add( std::shared_ptr<Building>( new Building( BELT, XY(1,1), 0 ) ) );
         }
         BluePrint a = bluePrintFor( Shape::fromCode( recipe.a ), finalTarget, mapping * recipe.mappingForA() );
         ret.add( a, XY(0,3) );
      }
      if ( recipe.op == ROTATE_1 || recipe.op == ROTATE_2 || recipe.op == ROTATE_3 )
      {
         BuildingType b = recipe.op == ROTATE_1 ? ROTATOR_1 : recipe.op == ROTATE_2 ? ROTATOR_2 : ROTATOR_3;
         ret.add( std::shared_ptr<Building>( new Building( b, XY(0,0), 0 ) ) );
         BluePrint a = bluePrintFor( Shape::fromCode( recipe.a ), finalTarget, mapping * recipe.mappingForA() );
         ret.add( a, XY(0,1) );
      }

      return ret;
   }

   void writeToFile( const string& filename )
   {
      ofstream f(filename, std::ios::binary);
      f.write( (const char*) _Recipes.data(), _Recipes.size()*sizeof(Recipe) );
      trace << "wrote recipes here: " << filename << endl;
   }
   bool loadFromFile( const string& filename )
   {
      ifstream f(filename, std::ios::binary);
      f.read( (char*) _Recipes.data(), _Recipes.size()*sizeof(Recipe) );
      return f.good();
   }

public:
   std::vector<Recipe> _Recipes;
};

class PossibleShapes
{
public:
   PossibleShapes()
   {
      _IsPossible.resize( 1<<16, false );
   }
   bool operator[]( int index ) const { return _IsPossible[index]; }
   void setIsPossible( int index, bool value ) { _IsPossible[index] = value; }

   void writeToFile( const string& filename )
   {
      ofstream f( filename, std::ios::binary );
      for ( int i = 0; i < (int) _IsPossible.size(); i++ )
      {
         uint8_t x = _IsPossible[i] ? 1 : 0;
         f.write( (const char*) &x, 1 );
      }
      trace << "wrote possible shapes here: " << filename << endl;
   }

public:
   vector<bool> _IsPossible;
};



bool isStackable( const std::vector<int>& v )
{
   if ( v[1] && !v[0] ) return false;
   if ( v[2] && !v[1] ) return false;
   if ( v[3] && !v[2] ) return false;
   return true;
}

bool canBeMadeFromTwoHalves( const Shape& shape )
{
   bool stack3 = isStackable( { shape.layers[0].b & 3, shape.layers[1].b & 3, shape.layers[2].b & 3, shape.layers[3].b & 3 } );
   bool stack12= isStackable( { shape.layers[0].b &12, shape.layers[1].b &12, shape.layers[2].b &12, shape.layers[3].b &12 } );
   bool stack6 = isStackable( { shape.layers[0].b & 6, shape.layers[1].b & 6, shape.layers[2].b & 6, shape.layers[3].b & 6 } );
   bool stack9 = isStackable( { shape.layers[0].b & 9, shape.layers[1].b & 9, shape.layers[2].b & 9, shape.layers[3].b & 9 } );
   return stack3 && stack12
      || stack6 && stack9;
}

void generateRecipesFile()
{
   const int ROTATE_COST = 0;
   const int CUT_COST = 1;
   const int STACK_COST = 1;

   std::unordered_set<uint16_t> usedShapesSet = { 0 };

   std::vector<std::pair<Shape, int>> allShapes;
   std::vector<std::pair<Shape, int>> allCanonicalShapes;
   std::vector<Shape> shapesWithCost[60];
   std::vector<Shape> canonicalShapesWithCost[60];

   std::vector<std::deque<Shape>> q( 1000 );

   Recipes recipes;
   std::vector<int> bestCostForShape( 1<<16, 99999999 );
   PossibleShapes possibleShapes;

   auto addShapeToQ = [&]( const Shape& shape, Op op, int cost, uint16_t codeA, uint16_t codeB ) {
      //if ( shape.numLayers() <= 2 )
      //   cost = 1;

      if ( cost >= bestCostForShape[shape.code()] )
         return;
      bestCostForShape[shape.code()] = cost;
      recipes[shape.code()] = { codeA, codeB, op };
      q[cost].push_back( shape );
   };

   for ( int i = 1; i <= 1; i++ )
   {
      addShapeToQ( Shape::fromCode( i ), RAW, 0, 0, 0 );
   }

   for ( int cost = 0; cost < (int) q.size(); cost++ )
   {
      if ( q[cost].empty() )
         continue;

      trace << "cost = " << cost << " allShapes.size() == " << allShapes.size() << endl;
      trace << "cost = " << cost << " allCanonicalShapes.size() == " << allCanonicalShapes.size() << endl;

      while ( !q[cost].empty() )
      {
         Shape shape = q[cost].front();
         q[cost].pop_front();

         if ( !usedShapesSet.insert( shape.code() ).second )
            continue;

         possibleShapes.setIsPossible( shape.code(), true );
         allShapes.push_back( { shape, cost } );
         if ( shape.isCanonical() )
            allCanonicalShapes.push_back( { shape, cost } );
         if ( shape.isCanonical() )
            canonicalShapesWithCost[cost].push_back( shape );
         shapesWithCost[cost].push_back( shape );

         ////Cu------:--Cu--Cu:Cu--Cu--:--Cu--Cu
         //if ( shape.code() == 0b1010'0101'1010'0001 )
         //   trace << recipeFor( shape, "" ) << endl;

         //if ( shape.code() == 0b1111'1111 )
         //if ( shape.code() == 0b0010'0001 )
         //if ( shape.code() == 0b1010'0101'1010'0001 )
         //if ( shape.code() == 0b1001'0110'1100'0011 )
         //if ( shape.code() == 0b0110'1010'0011'0100 ) // ----Cu--:CuCu----:--Cu--Cu:--CuCu-- (needs 3 cuts)
         //if ( shape.code() == 0b0101'1010'0101'1000 ) // ------Cu:Cu--Cu--:--Cu--Cu:Cu--Cu--
         //if ( shape.code() == 0b0001'0001'0011'1000 ) // ------Cu:CuCu----:Cu------:Cu------
         //if ( shape.code() == targetShapeCode )
         //{
         //   trace << recipeFor( shape, "" ) << endl;
         //   trace << "blueprint for " << shape.str() << endl;
         //   trace << bluePrintFor( shape, Mapping::identity() ).toJson() << endl;

         //}


         if ( allShapes.size() % 1000 == 0 )
            trace << allShapes.size() << endl;


         addShapeToQ( shape.rotated(), ROTATE_1, cost+ROTATE_COST, shape.code(), 0 );
         addShapeToQ( shape.rotated().rotated(), ROTATE_2, cost+ROTATE_COST, shape.code(), 0 );
         addShapeToQ( shape.rotated().rotated().rotated(), ROTATE_3, cost+ROTATE_COST, shape.code(), 0 );

         addShapeToQ( shape.cutLeft(), CUT_LEFT, cost+CUT_COST, shape.code(), 0 );
         addShapeToQ( shape.cutRight(), CUT_RIGHT, cost+CUT_COST, shape.code(), 0 );

         for ( const auto& b : allShapes )
         {
            int stackedCost = cost+b.second+STACK_COST;
            //int stackedCost = cost;
            addShapeToQ( stack( shape, b.first ), STACK, stackedCost, shape.code(), b.first.code() );
            addShapeToQ( stack( b.first, shape ), STACK, stackedCost, b.first.code(), shape.code() );
         }
      }

      trace << "#shapes with cost " << cost << " = " << shapesWithCost[cost].size() << endl;
      trace << "#canonical shapes with cost " << cost << " = " << canonicalShapesWithCost[cost].size() << endl;

      if ( canonicalShapesWithCost[cost].size() <= 100 )
         for ( const Shape& shape : canonicalShapesWithCost[cost] )
            trace << shape.str() << endl;
   }


   string filename = "recipes_" + to_string( ROTATE_COST ) + "_" + to_string( CUT_COST ) + "_" + to_string( STACK_COST ) + ".bin";
   recipes.writeToFile( filename );
   possibleShapes.writeToFile( "shape_is_possible.bin" );
}

int main()
{
   //generateRecipesFile(); // this generates "recipes_0_1_1.bin"

   Recipes recipes( "recipes_0_1_1.bin" );

   //string TARGET = "CbCuCbCu:Sr------:--CrSrCr:CwCwCwCw";
   string TARGET = "------Cr:CgCb----:Cp------:Cy------";
   BluePrint bluePrint = recipes.bluePrintFor( TARGET );

   trace << bluePrint.toJson() << endl;


   //{
   //   int numStackableFromSingleLayers = 0;
   //   int numFromTwoHalves = 0;
   //   int numConstructable = 0;
   //   for ( int c = 0; c < (1 << 16); c++ ) if ( g_shapeIsPossible[c] && Shape::fromCode( c ).isCanonical() )
   //   {
   //      bool floating = Shape::fromCode( c ).hasFloatingLayer();
   //      bool canBeMadeFrom2Halves = canBeMadeFromTwoHalves( Shape::fromCode( c ) );

   //      if ( !floating ) numStackableFromSingleLayers++;
   //      if ( canBeMadeFrom2Halves ) numFromTwoHalves++;
   //      if ( canBeMadeFrom2Halves || !floating ) numConstructable++;
   //      else
   //         trace << Shape::fromCode( c ).str() << endl;
   //   }
   //   trace << "numStackableFromSingleLayers = " << numStackableFromSingleLayers << endl;
   //   trace << "numFromTwoHalves = " << numFromTwoHalves << endl;
   //   trace << "numConstructable = " << numConstructable << endl;
   //}


   return 0;
}
