/** \file uniformGrid.h

    \brief A container for fast spatial lookups and insertions

    \author Copyright 2009-2016 Dr. Michael Jason Gourlay; All rights reserved.  Contact me at mijagourlay.com for licensing.
        - http://www.mijagourlay.com/
*/

#include <cstring>

#include "Core/Math/mat33.h"
#include "Core/SpatialPartition/uniformGrid.h"
#include "Core/SpatialPartition/nestedGrid.h"

#include "VortonFluid/vorton.h"
#include "VortonFluid/vortonClusterAux.h"

namespace Math {
    inline bool RResembles(int a, int b, float r) { return Resembles((float)a, (float)b , r); }
    inline bool RResembles(float a, float b, float r) { return Resembles(a, b , r); }
    inline bool RResembles(unsigned int a, unsigned int b , float r) { return Resembles((float)a, (float)b , r);}
    inline bool RResembles(unsigned long a, unsigned long b , float r) { return Resembles((float)a, (float)b , r);}
    inline bool RResembles(const std::vector<unsigned int>& a, const std::vector<unsigned int>& b, float r) { return (int)a.size() == (int)b.size(); }
    inline bool RResembles(const VortonClusterAux&a, const VortonClusterAux&b, float r) { return true; }
    inline bool RResembles(const Vec3 &a, const Vec3 &b, float r) { return a.Resembles(b,r); }
    inline bool RResembles(const Mat33 &a, const Mat33 &b, float r) { return a.Resembles(b,r); }
    inline bool RResembles(const Vorton &a, const Vorton &b, float r) { return true; }

}
    inline bool IsInf(const std::vector<unsigned int>&) { return false; }

void GetChildClusterMinCornerIndexS( unsigned clusterMinIndices[3] , const unsigned long decimations[3] , const unsigned indicesOfParentCell[3] )
{
    clusterMinIndices[ 0 ] = indicesOfParentCell[ 0 ] * (unsigned int)decimations[ 0 ] ;
    clusterMinIndices[ 1 ] = indicesOfParentCell[ 1 ] * (unsigned int)decimations[ 1 ] ;
    clusterMinIndices[ 2 ] = indicesOfParentCell[ 2 ] * (unsigned int)decimations[ 2 ] ;
}

// -----------------------------------------------------------------------------
// Dummy Operators for std::vector<unsigned int>
// Required to satisfy the compiler for unused UniformGrid math template methods
// -----------------------------------------------------------------------------

inline std::vector<unsigned int> operator*(const std::vector<unsigned int>& v, float) { return v; }
inline std::vector<unsigned int> operator*(float, const std::vector<unsigned int>& v) { return v; }
inline std::vector<unsigned int> operator*(const std::vector<unsigned int>& a, const std::vector<unsigned int>&) { return a; }
inline std::vector<unsigned int> operator+(const std::vector<unsigned int>& a, const std::vector<unsigned int>&) { return a; }
inline std::vector<unsigned int> operator-(const std::vector<unsigned int>& a, const std::vector<unsigned int>&) { return a; }

inline std::vector<unsigned int>& operator+=(std::vector<unsigned int>& a, const std::vector<unsigned int>&) { return a; }
inline std::vector<unsigned int>& operator-=(std::vector<unsigned int>& a, const std::vector<unsigned int>&) { return a; }
inline std::vector<unsigned int>& operator*=(std::vector<unsigned int>& a, float) { return a; }
inline std::vector<unsigned int>& operator/=(std::vector<unsigned int>& a, float) { return a; }

inline bool operator<(const std::vector<unsigned int>&, const std::vector<unsigned int>&) { return false; }
inline bool operator>(const std::vector<unsigned int>&, const std::vector<unsigned int>&) { return false; }

inline bool IsNan(const std::vector<unsigned int>&) { return false; }

/** Base class for uniform grid.

    The shape of this grid is such that the "minimal corner" point
    resides at indices {0,0,0} and the "maximal corner" point
    resides at indices {Nx-1,Ny-1,Nz-1}.

    The number of /points/ in each direction i is N_i.

    A cell is defined by the 8 points that lie at its corners.

    This also implies that the grid must have at least 2 points in
    each direction.

    The number of /cells/ in each direction i is (N_i-1).

    The size of a side i of each cell is therefore
        - s_i = (vMax-vMin)_i / (N_i-1) .

    \verbatim
                  vMin
                   0       1      ...     Nx-1
                  (*)------*-------*-------* 0
                 ./|       |       |       |
                ./ |       |       |       |
               .*  |       |       |       |
               /|  *-------*-------*-------* 1
              / | /|       |       |       |
        Nz-1 *  |/ |       | cell  |       |
             |  *  |       |       |       | .
             | /|  *-------*-------*-------* .
             |/ | /|       |       |       | .
             *  |/ |       |       |       |
             |  *  |       |       |       |
             | /|  *-------*-------*-------* Ny-1
             |/ | /       /       /       /
             *  |/       /       /       /
             |  *-------*-------*-------*
             | /       /       /       /
             |/       /       /       /
             *-------*-------*------(*)vMax
    \endverbatim


*/




        /** Construct an empty UniformGridGeometry.

            \see Clear
        */
        UniformGridGeometry::UniformGridGeometry()
            : mMinCorner( 0.0f , 0.0f , 0.0f )
            , mGridExtent( 0.0f , 0.0f , 0.0f )
            , mCellExtent( 0.0f , 0.0f , 0.0f )
            , mCellsPerExtent( 0.0f , 0.0f , 0.0f )
        {
            mNumPoints[ 0 ] = mNumPoints[ 1 ] = mNumPoints[ 2 ] = 0 ;
        }


        /** Construct a uniform grid that fits the given geometry.

            \see Clear, DefineShape
        */
        UniformGridGeometry::UniformGridGeometry( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 )
        {
            DefineShape( uNumElements , vMin , vMax , bPowerOf2 ) ;
        }

        /// Return minimal corner of this UniformGrid.
        const Vec3 & UniformGridGeometry::GetMinCorner() const { return mMinCorner ; }


        /// Return maximal corner of this UniformGrid.
        Vec3        UniformGridGeometry::GetMaxCorner() const { return GetMinCorner() + GetExtent() ; }


        /** Return whether the given query point lies inside the boundaries of this grid.
        */
        bool    UniformGridGeometry::Encompasses( const Vec3 & queryPoint ) const
        {
            return ( queryPoint >= GetMinCorner() ) && ( queryPoint <= GetMaxCorner() ) ;
        }


        /// Return center position that this UniformGrid contains.
        Vec3        UniformGridGeometry::GetCenter() const
        {
            return ( GetMinCorner() + GetMaxCorner() ) * 0.5f ;
        }


        /** Return whether the shape of this object matches that of the given.

            \param that - Other UniformGridGeometry object

            \return true if the shape of this object matches that of that.

        */
        bool UniformGridGeometry::ShapeMatches( const UniformGridGeometry & that ) const
        {
            return      ( GetNumPoints( 0 ) == that.GetNumPoints( 0 ) )
                    &&  ( GetNumPoints( 1 ) == that.GetNumPoints( 1 ) )
                    &&  ( GetNumPoints( 2 ) == that.GetNumPoints( 2 ) )
                    &&  ( GetMinCorner()    == that.GetMinCorner()    )
                    &&  ( GetExtent()       == that.GetExtent()       ) ;
        }



        bool UniformGridGeometry::operator==( const UniformGridGeometry & that ) const
        {
            return ShapeMatches( that )
                && mGridExtent      == that.mGridExtent
                && mCellExtent      == that.mCellExtent
                && mCellsPerExtent  == that.mCellsPerExtent ;
        }




        /** Define the shape a uniform grid such that it fits the given geometry.

            \param uNumElements - number of elements this container will contain.

            \param vMin - minimal coordinate of axis-aligned bounding box.

            \param vMax - maximal coordinate of axis-aligned bounding box.

            \param bPowerOf2 - whether to make each grid dimension a power of 2.
                Doing so simplifies grid subdivision, if this grid will be used
                in a hierarchical grid.

            This makes a uniform grid of cells, where each cell is the same size
            and the side of each cell is nearly the same size.  If the cells are
            3-dimensional then that means each cell is a box, nearly a cube.
            The number of dimensions of the region depends on the actual size of
            the region.  If any size component is smaller than a small threshold
            then this class considers that component to be zero, and reduces the
            dimensionality of the region.  For example, if the region size is
            (2,3,0) then this class considers the region to have 2 dimensions
            (x and y) since the z size is zero.  In this example, the cells
            would be nearly square rectangles (instead of boxes).

        */
        void UniformGridGeometry::DefineShape( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 )
        {
            if( uNumElements > 0 )
            {   // This grid contains elements.
                mMinCorner  = vMin ;
                static const float Nudge = 1.0f + FLT_EPSILON ;  // slightly expand size to ensure robust containment even with roundoff
                mGridExtent     = ( vMax - vMin ) * Nudge ;

                Vec3 vSizeEffective( GetExtent() ) ;
                int numDims = 3 ;   // Number of dimensions to region.
                if( 0.0f == vSizeEffective.x )
                {   // X size is zero so reduce dimensionality
                    vSizeEffective.x = 1.0f ; // This component will not contribute to the total region volume/area/length.
                    mGridExtent.x = 0.0f ;
                    -- numDims ;
                }
                if( 0.0f == vSizeEffective.y )
                {   // Y size is zero so reduce dimensionality
                    vSizeEffective.y = 1.0f ; // This component will not contribute to the total region volume/area/length.
                    mGridExtent.y = 0.0f ;
                    -- numDims ;
                }
                if( 0.0f == vSizeEffective.z )
                {   // Z size is zero so reduce dimensionality
                    vSizeEffective.z = 1.0f ; // This component will not contribute to the total region volume/area/length.
                    mGridExtent.z = 0.0f ;
                    -- numDims ;
                }
                ASSERT( numDims > 0 ) ;

                // Compute region volume, area or length (depending on dimensionality).
                const float volume                  = vSizeEffective.x * vSizeEffective.y * vSizeEffective.z ;
                const float invCellVolumeCubeRoot   = powf( volume / float( uNumElements ) , -1.0f / float( numDims ) ) ; // Approximate size of each cell in grid.
                // Compute number of cells in each direction of uniform grid.
                // Choose grid dimensions to fit as well as possible, so that the total number
                // of grid cells is nearly the total number of elements in the contents.
                size_t numCells[3] = { Max2( size_t( 1 ) , static_cast< size_t >( GetExtent().x * invCellVolumeCubeRoot + 0.5f ) ) ,
                                       Max2( size_t( 1 ) , static_cast< size_t >( GetExtent().y * invCellVolumeCubeRoot + 0.5f ) ) ,
                                       Max2( size_t( 1 ) , static_cast< size_t >( GetExtent().z * invCellVolumeCubeRoot + 0.5f ) ) } ;
                if( bPowerOf2 )
                {   // Choose number of gridcells to be powers of 2.
                    // This will simplify subdivision in a NestedGrid.
                    numCells[ 0 ] = NearestPowerOfTwo( numCells[ 0 ] ) ;
                    numCells[ 1 ] = NearestPowerOfTwo( numCells[ 1 ] ) ;
                    numCells[ 2 ] = NearestPowerOfTwo( numCells[ 2 ] ) ;
                }
                while( numCells[ 0 ] * numCells[ 1 ] * numCells[ 2 ] >= uNumElements * 8 )
                {   // Grid capacity is excessive.
                    // This can occur when the trial numCells is below 0.5 in which case the integer arithmetic loses the subtlety.
                    numCells[ 0 ] = Max2< size_t >( 1 , numCells[0] / 2 ) ;
                    numCells[ 1 ] = Max2< size_t >( 1 , numCells[1] / 2 ) ;
                    numCells[ 2 ] = Max2< size_t >( 1 , numCells[2] / 2 ) ;
                }
                mNumPoints[ 0 ] = numCells[ 0 ] + 1 ; // Increment to obtain number of points.
                mNumPoints[ 1 ] = numCells[ 1 ] + 1 ; // Increment to obtain number of points.
                mNumPoints[ 2 ] = numCells[ 2 ] + 1 ; // Increment to obtain number of points.

                PrecomputeSpacing() ;
            }
            else
            {   // This grid contains no elements.
                Clear() ;
            }
        }


        /** Scale given grid.

            \param src - Source uniform grid upon which to base dimensions of this one

            \param scale - amount by which to scale each axis of grid

            \note The number of cells is also scaled.

        */
        void UniformGridGeometry::Scale( const UniformGridGeometry & src , const Vec3 & scale )
        {
            mGridExtent     = Vec3( src.mGridExtent.x * scale.x , src.mGridExtent.y * scale.y , src.mGridExtent.z * scale.z ) ;
            mMinCorner      = Vec3( src.mMinCorner.x  * scale.x , src.mMinCorner.y  * scale.y , src.mMinCorner.z  * scale.z ) ;
            mNumPoints[ 0 ] = int( float( src.GetNumCells( 0 ) ) * scale.x ) + 1 ;
            mNumPoints[ 1 ] = int( float( src.GetNumCells( 1 ) ) * scale.y ) + 1 ;
            mNumPoints[ 2 ] = int( float( src.GetNumCells( 2 ) ) * scale.z ) + 1 ;
            PrecomputeSpacing() ;
        }



        /** Create a higher-resolution uniform grid based on another.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iExpansion  Amount by which to increase the number of grid cells in each dimension.
                Typically this would be a power of 2.

            \note The number of cells is increased.  The number of points is different.

        */
        void UniformGridGeometry::Expand( const UniformGridGeometry & src , int iExpansions[ 3 ] )
        {
            mGridExtent         = src.mGridExtent ;
            mMinCorner          = src.mMinCorner ;
            mNumPoints[ 0 ]     = src.GetNumCells( 0 ) * iExpansions[ 0 ] + 1 ;
            mNumPoints[ 1 ]     = src.GetNumCells( 1 ) * iExpansions[ 1 ] + 1 ;
            mNumPoints[ 2 ]     = src.GetNumCells( 2 ) * iExpansions[ 2 ] + 1 ;
            PrecomputeSpacing() ;
        }




        /** Create a higher-resolution uniform grid based on another.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iExpansion  Amount by which to increase the number of grid cells in each dimension.
                Typically this would be a power of 2.

            \note The number of cells is increased.  The number of points is different.

        */
        void UniformGridGeometry::Expand( const UniformGridGeometry & src , int expansion )
        {
            int expansions[ 3 ] = { expansion , expansion , expansion } ;
            Expand( src , expansions ) ;
        }




        /** Make this a lower-resolution uniform grid based the given source grid.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iDecimation  Amount by which to reduce the number of grid cells in each dimension.
                Typically this would be 2.

            \note The number of cells is decimated.  The number of points is different.

        */
        void UniformGridGeometry::Decimate( const UniformGridGeometry & src , int iDecimation )
        {
            mGridExtent         = src.mGridExtent ;
            mMinCorner          = src.mMinCorner ;
            mNumPoints[ 0 ]     = src.GetNumCells( 0 ) / iDecimation + 1 ;
            mNumPoints[ 1 ]     = src.GetNumCells( 1 ) / iDecimation + 1 ;
            mNumPoints[ 2 ]     = src.GetNumCells( 2 ) / iDecimation + 1 ;
            if( iDecimation > 1 )
            {   // Decimation could reduce dimension and integer arithmetic could make value be 0, which is useless if src contained any data.
                mNumPoints[ 0 ] = Max2( size_t( 2 ) , (unsigned long)GetNumPoints( 0 ) ) ;
                mNumPoints[ 1 ] = Max2( size_t( 2 ) , (unsigned long)GetNumPoints( 1 ) ) ;
                mNumPoints[ 2 ] = Max2( size_t( 2 ) , (unsigned long)GetNumPoints( 2 ) ) ;
            }
            PrecomputeSpacing() ;
        }



        /** Copy shape information from another UniformGrid into this one.
        */
        void UniformGridGeometry::CopyShape( const UniformGridGeometry & src )
        {
            Decimate( src , 1 ) ;
        }




        /** Use shape information from another UniformGrid and contraints to determine shape of this grid.
        */
        void UniformGridGeometry::FitShape( const UniformGridGeometry & src , const float minCellSpacing )
        {
            CopyShape( src ) ; // Kick-start geometry using given shape.

            const float * cellSpacings = reinterpret_cast< const float * >( & GetCellSpacing() ) ;
            for( int axis = 0 ; axis < 3 ; ++ axis )
            {
                int decimation = 1 ;
                while(      ( cellSpacings[ axis ] < minCellSpacing )
                        &&  ( GetNumCells( axis ) > 2 ) )
                {
                    decimation *= 2 ;
                    mNumPoints[ axis ] = Max2( size_t( 2 ) , (unsigned long)src.GetNumCells( axis ) / decimation + 1 ) ;
                    PrecomputeSpacing() ;
                }
            }
        }


        /** Get world-space dimensions of UniformGridGeometry.
        */
        const Vec3 & UniformGridGeometry::GetExtent() const { return mGridExtent ; }


        /** Whether this geometry has zero extent.
        */
        bool UniformGridGeometry::HasZeroExtent() const { return GetExtent() == Vec3( 0.0f , 0.0f , 0.0f ) ; }


        /** Get reciprocal of cell spacing.
        */
        const Vec3 & UniformGridGeometry::GetCellsPerExtent() const { return mCellsPerExtent ; }


        /** Get number of grid points along the given dimension.

            \param index - dimension queried, where 0 means x, 1 means y and 2 means z.

            \note The number of cells in each direction i is GetNumPoints(i) - 1.

        */
        const unsigned    UniformGridGeometry::GetNumPoints( const unsigned & index ) const    { return mNumPoints[ index ] ; }


        /** Get number of grid cells along the given dimension.

            \param index - dimension queried, where 0 means x, 1 means y and 2 means z.

            \see GetNumPoints

        */
        unsigned long        UniformGridGeometry::GetNumCells( const unsigned & index ) const
        {
            ASSERT( GetNumPoints( index ) >= 1 ) ;
            return GetNumPoints( index ) - 1 ;
        }


        /** Get total number of gridpoints.

            \note This returns the number of gridpoints defined by the geometry, which is separate
                    from the capacity of the container.  When the container has been initialized,
                    these should be the same, but prior to initializing the container,
                    GetGridCapacity can be non-zero even when the container size is zero.

        */
        unsigned UniformGridGeometry::GetGridCapacity() const { return GetNumPoints( 0 ) * GetNumPoints( 1 ) * GetNumPoints( 2 ) ; }


        /** Return extent (in world units) of a grid cell.
        */
        const Vec3 &    UniformGridGeometry::GetCellSpacing() const  { return mCellExtent ; }


        /** Return volume (in world units) of entire grid.
        */
        const float UniformGridGeometry::GetVolume() const   { return GetExtent().x * GetExtent().y * GetExtent().z ; }


        /** Return volume (in world units) of a grid cell.
        */
        const float UniformGridGeometry::GetCellVolume() const   { return mCellExtent.x * mCellExtent.y * mCellExtent.z ; }


        /** Return extent (in world units) of a grid cell.
        */
        const Vec3  UniformGridGeometry::GetCellCenter( int ix , int iy , int iz ) const
        {
            const Vec3 vOffset(   float( ix ) * GetCellSpacing().x
                                , float( iy ) * GetCellSpacing().y
                                , float( iz ) * GetCellSpacing().z ) ;
            const Vec3 vMin( GetMinCorner() + vOffset ) ;
            const Vec3 vCenter( vMin + GetCellSpacing() * 0.5f ) ;
            return vCenter ;
        }


        /** Compute indices into contents array of a point at a given position.

            \param vPosition - position of a point.  It must be within the region of this container.

            \param indices - Indices into contents array of a point at vPosition.

            \see IndicesFromOffset, PositionFromOffset, OffsetOfPosition, PositionFromIndices

            \note Derived class defines the actual contents array.

        */
        void UniformGridGeometry::IndicesOfPosition( unsigned long indices[4] , const Vec3 & vPosition ) const
        {
            // Notice the peculiar test here.  vPosition may lie slightly outside of the extent given by vMax.
            // Review the geometry described in the class header comment.
            Vec3 vPosRel( vPosition - GetMinCorner() ) ;   // position of given point relative to container region
            Vec3 vIdx( vPosRel.x * GetCellsPerExtent().x , vPosRel.y * GetCellsPerExtent().y , vPosRel.z * GetCellsPerExtent().z ) ;
        #if 1   // Original code.
            indices[0] = unsigned( vIdx.x ) ;
            indices[1] = unsigned( vIdx.y ) ;
            indices[2] = unsigned( vIdx.z ) ;
        #elif 0 // Optimization 1: change control word once for all 3 conversions
            const WORD OldCtrlWord = Changex87FloatingPointToTruncate() ;
            indices[0] = StoreFloatAsInt( vIdx.x ) ;
            indices[1] = StoreFloatAsInt( vIdx.y ) ;
            indices[2] = StoreFloatAsInt( vIdx.z ) ;
            SetFloatingPointControlWord( OldCtrlWord ) ;
        #elif 0 // Optimization 2: Use SSE instructions that do not require 16-byte alignment
            _asm {
                movdqu      xmm0    , vIdx
                cvttps2dq   xmm0    , xmm0
                movdqu      indices , xmm0
            }
        #elif 0 // Optimization 3: Use SSE instructions that assume 16-byte alignment
            _asm {
                cvttps2dq   xmm0    , vIdx
                movdqa      indices , xmm0
            }
        #elif 0 // Optimization 4: use approximation.  Faster, but gives incorrect results sometimes.
            indices[0] = FastIntFromFloatApproximate( vIdx.x ) ;
            indices[1] = FastIntFromFloatApproximate( vIdx.y ) ;
            indices[2] = FastIntFromFloatApproximate( vIdx.z ) ;
        #endif
            ASSERT( indices[0] < GetNumPoints( 0 ) ) ;
            ASSERT( indices[1] < GetNumPoints( 1 ) ) ;
            ASSERT( indices[2] < GetNumPoints( 2 ) ) ;
            ASSERT( ! sInterpolating || ( indices[0] < GetNumCells( 0 ) ) ) ; // DO NOT SUBMIT
            ASSERT( ! sInterpolating || ( indices[1] < GetNumCells( 1 ) ) ) ; // DO NOT SUBMIT
            ASSERT( ! sInterpolating || ( indices[2] < GetNumCells( 2 ) ) ) ; // DO NOT SUBMIT
        }


        /** Get indices of nearest grid point.

            \param indices      (out) Indices of nearest grid point.

            \param vPosition    (in) Position being queried.

            This contrasts with IndicesOfPosition, which gets the indices of the nearest grid *cell*,
            which is tantamount to getting the minimal corner gridpoint of that cell.
            IndicesOfPosition is like rounding down whereas this routine is like
            rounding to nearest.
        */
        void UniformGridGeometry::IndicesOfNearestGridPoint( unsigned long indices[4] , const Vec3 & vPosition ) const
        {
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = true ) ;
            IndicesOfPosition( indices , vPosition ) ;
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = false ) ;
            ASSERT( indices[0] < GetNumCells( 0 ) ) ;
            ASSERT( indices[1] < GetNumCells( 1 ) ) ;
            ASSERT( indices[2] < GetNumCells( 2 ) ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            const Vec3  vDiff   = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
            const Vec3  tween   = Vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ;
            if( tween.x >= 0.5f ) ++ indices[ 0 ] ;
            if( tween.y >= 0.5f ) ++ indices[ 1 ] ;
            if( tween.z >= 0.5f ) ++ indices[ 2 ] ;
        }


        /** Compute offset into contents array of a point at a given position.

            \param vPosition - position of a point.  It must be within the region of this container.

            \return Offset into contents array of a point at vPosition.

            \see IndicesFromOffset, PositionFromOffset, PositionFromIndices

            \note Derived class defines the actual contents array.

        */
        unsigned    UniformGridGeometry::OffsetOfPosition( const Vec3 & vPosition ) const
        {
            unsigned long indices[4] ;
            IndicesOfPosition( indices , vPosition ) ;
            const unsigned offset = indices[0] + GetNumPoints( 0 ) * ( indices[1] + GetNumPoints( 1 ) * indices[2] ) ;
            return offset ;
        }


        /** Compute position of minimal corner of grid cell with given indices.

            \param vPosition    (out) Position of minimal corner of grid cell.

            \param indices      (in) Grid cell indices.

            \note Rarely would you want to compute position from indices in this
                    way. Typically, this kind of computation occurs inside a
                    triply-nested loop, in which case the procedure should
                    compute each component separately.  Furthermore, such a
                    routine would cache GetCellSpacing instead of computing it
                    each iteration.

        */
        void    UniformGridGeometry::PositionFromIndices( Vec3 & vPosition , const unsigned long indices[3] ) const
        {
            vPosition.x = GetMinCorner().x + float( indices[0] ) * GetCellSpacing().x ;
            vPosition.y = GetMinCorner().y + float( indices[1] ) * GetCellSpacing().y ;
            vPosition.z = GetMinCorner().z + float( indices[2] ) * GetCellSpacing().z ;
        }


        /** Compute position of minimal corner of grid cell with given indices.

            \param ix   X index of grid cell.
            \param iy   Y index of grid cell.
            \param iz   Z index of grid cell.

            \return Position of minimal corner of grid cell.
        */
        Vec3    UniformGridGeometry::PositionFromIndices( unsigned long ix , unsigned long iy , unsigned long iz ) const
        {
            Vec3 vPosition( GetMinCorner().x + float( ix ) * GetCellSpacing().x
                          , GetMinCorner().y + float( iy ) * GetCellSpacing().y
                          , GetMinCorner().z + float( iz ) * GetCellSpacing().z ) ;
            return vPosition ;
        }


        /** Compute position of minimal corner of grid cell with given indices.

            \param indices      (in) Grid cell indices.

            \return Position of minimal corner of grid cell.

        */
        Vec3    UniformGridGeometry::PositionFromIndices( const unsigned long indices[3] ) const
        {
            Vec3 vPosition( GetMinCorner().x + float( indices[0] ) * GetCellSpacing().x
                          , GetMinCorner().y + float( indices[1] ) * GetCellSpacing().y
                          , GetMinCorner().z + float( indices[2] ) * GetCellSpacing().z ) ;
            return vPosition ;
        }


        /** Compute X,Y,Z grid cell indices from offset into contents array.

            \param indices - Individual X,Y,Z component grid cell indices.

            \param offset - Offset into mContents.
        */
        void    UniformGridGeometry::IndicesFromOffset( unsigned long indices[3] , const unsigned long & offset ) const
        {
            indices[2] = offset / ( GetNumPoints(0) * GetNumPoints(1) ) ;
            indices[1] = ( offset - indices[2] * GetNumPoints(0) * GetNumPoints(1) ) / GetNumPoints(0) ;
            indices[0] = offset - GetNumPoints(0) * ( indices[1] + GetNumPoints(1) * indices[2] ) ;
        }


        /** Get position of grid cell minimum corner.

            \param vPos     Position of grid cell minimum corner.

            \param offset   Offset into contents array.

            Each grid cell spans a region (whose size is given by GetCellSpacing)
            starting at a location which this routine returns.  So the grid cell
            with the given offset spans the region from vPos (as this routine
            assigns) to vPos + GetCellSpacing().

            \note Derived class provides actual contents array.

        */
        void    UniformGridGeometry::PositionFromOffset( Vec3 & vPos , const unsigned long & offset )
        {
            unsigned long indices[3] ;
            IndicesFromOffset( indices , offset ) ;
            vPos.x = GetMinCorner().x + float( indices[0] ) * GetCellSpacing().x ;
            vPos.y = GetMinCorner().y + float( indices[1] ) * GetCellSpacing().y ;
            vPos.z = GetMinCorner().z + float( indices[2] ) * GetCellSpacing().z ;
        }


        /** Get offset into contents array given indices.

            \param ix   X index of grid point.
            \param iy   Y index of grid point.
            \param iz   Z index of grid point.

            \return offset into contents array

        */
        size_t UniformGridGeometry::OffsetFromIndices( size_t ix , size_t iy , size_t iz ) const
        {
            return ix + GetNumPoints(0) * ( iy + GetNumPoints(1) * iz ) ;
        }


        /** Get offset into contents array given indices.

            \param indices - indices specifying a grid cell

            \return offset into contents array

            \note Typically this routine would not be efficient to use, except for special cases.
                    Often, one writes a triple-nested loop iterating over each
                    component of indices, in which case it is more efficient
                    to compute the z and y terms of the offset separately and 
                    combine them with the x term in the inner-most loop.
                    This routine is useful primarily when there is no coherence
                    between the indices of this iteration and the previous or next.

            \note Derived class provides actual contents array.

        */
        size_t UniformGridGeometry::OffsetFromIndices( const size_t indices[4] ) const
        {
            return OffsetFromIndices( indices[ 0 ] , indices[ 1 ] , indices[ 2 ] ) ;
        }

        size_t UniformGridGeometry::OffsetFromIndices( const unsigned int indices[4] ) const
        {
            return OffsetFromIndices( indices[ 0 ] , indices[ 1 ] , indices[ 2 ] ) ;
        }



        void UniformGridGeometry::PrecomputeSpacing()
        {
            mCellExtent.x       = GetExtent().x / float( GetNumCells( 0 ) ) ;
            mCellExtent.y       = GetExtent().y / float( GetNumCells( 1 ) ) ;
            mCellExtent.z       = GetExtent().z / float( GetNumCells( 2 ) ) ;
            mCellsPerExtent.x   = float( GetNumCells( 0 ) ) / GetExtent().x ;
            mCellsPerExtent.y   = float( GetNumCells( 1 ) ) / GetExtent().y ;
            if( 0.0f == GetExtent().z )
            {   // Avoid divide-by-zero for 2D domains that lie in the XY plane.
                mCellsPerExtent.z   = 1.0f / FLT_MIN ;
            }
            else
            {
                mCellsPerExtent.z   = float( GetNumCells( 2 ) ) / GetExtent().z ;
            }
        }




        /** Return the offset associated with the cell whose indices are second-to-last in each direction.

            Same as nx * ( ny * ( nz - 1 ) - 1 ) - 2.
        */
        unsigned UniformGridGeometry::GetOffsetOfPenultimateCell()
        {
            return (GetNumPoints(0)-2) + GetNumPoints(0) * ( (GetNumPoints(1)-2) + GetNumPoints(1) * (GetNumPoints(2)-2) ) ;
        }


        /** Clear out any existing shape information.
        */
        void UniformGridGeometry::Clear()
        {
            mMinCorner      =
            mGridExtent     =
            mCellExtent     =
            mCellsPerExtent = Vec3( 0.0f , 0.0f , 0.0f ) ;
            mNumPoints[ 0 ] = mNumPoints[ 1 ] = mNumPoints[ 2 ] = 0 ;
        }









#if USE_TBB
    /** Templated function object to down-sample
    */
            void UniformGrid_DownSample_TBB::operator() ( const tbb::blocked_range<size_t> & r ) const
            {   // Perform subset of down-sampling
                SetFloatingPointControlWord( mMasterThreadFloatingPointControlWord ) ;
                SetMmxControlStatusRegister( mMasterThreadMmxControlStatusRegister ) ;
                mLoResDst.DownSampleSlice( mHiResSrc , mAccuracyVersusSpeed , r.begin() , r.end() ) ;
            }
            UniformGrid_DownSample_TBB::UniformGrid_DownSample_TBB( UniformGrid & loResDst , const UniformGrid & hiResSrc , UniformGridGeometry::AccuracyVersusSpeedE accuracyVsSpeed )
                : mLoResDst( loResDst )
                , mHiResSrc( hiResSrc )
                , mAccuracyVersusSpeed( accuracyVsSpeed )
            {
                mMasterThreadFloatingPointControlWord = GetFloatingPointControlWord() ;
                mMasterThreadMmxControlStatusRegister = GetMmxControlStatusRegister() ;
            }


    /** Templated function object to up-sample
    */


            UniformGrid_UpSample_TBB::operator() ( const tbb::blocked_range<size_t> & r ) const
            {   // Perform subset of down-sampling
                SetFloatingPointControlWord( mMasterThreadFloatingPointControlWord ) ;
                SetMmxControlStatusRegister( mMasterThreadMmxControlStatusRegister ) ;
                mHiResDst.UpSampleSlice( mLoResSrc , mRegion , r.begin() , r.end() ) ;
            }
            UniformGrid_UpSample_TBB::UniformGrid_UpSample_TBB( UniformGrid & hiResDst , const UniformGrid & loResSrc , UniformGridGeometry::RegionE region )
                : mHiResDst( hiResDst )
                , mLoResSrc( loResSrc )
                , mRegion( region )
            {
                mMasterThreadFloatingPointControlWord = GetFloatingPointControlWord() ;
                mMasterThreadMmxControlStatusRegister = GetMmxControlStatusRegister() ;
            }
#endif


        /** Construct an empty UniformGrid.
            \see Initialize
        */
       template<class ItemT>
        UniformGrid<ItemT>::UniformGrid() : UniformGridGeometry() { }


        /** Construct a uniform grid container that fits the given geometry.
            \see Initialize
        */
       template<class ItemT>
        UniformGrid<ItemT>::UniformGrid( unsigned uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 )
            : UniformGridGeometry( uNumElements , vMin , vMax , bPowerOf2 )
        {
        }


        /** Copy shape from given uniform grid.
        */
       template<class ItemT>
        UniformGrid<ItemT>::UniformGrid( const UniformGridGeometry & that )
            : UniformGridGeometry( that )
        {
        }


        /** Copy constructor for empty uniform grids.

            This copy constructor does not copy contained contents.  A proper
            copy ctor should deep-copy all of its data in order to operate with
            containers, in particular STL vector) when it reallocates arrays.
            But reallocating these containers would entail massive memory moves,
            which although possible, would be inefficient.  Meanwhile, NestedGrid
            also pushes UniformGrids onto a vector, which requires using a copy
            ctor to initialize the new element.  So that code makes empty elements
            to push onto the vector, and arrange for this copy ctor to handle those
            empties properly.

            Meanwhile we want to catch any unintentional copies of actual data,
            so this method catches any attempt to copy populated UniformGrid objects.
        */
       template<class ItemT>
        UniformGrid<ItemT>::UniformGrid( const UniformGrid & that )
            : UniformGridGeometry( that )
        {
            //ASSERT( that.Empty() ) ; // Maybe you meant to use UniformGrid( const UniformGridGeometry & ) instead.
            this->operator=( that ) ;
        }

        template<class ItemT>
        UniformGrid<ItemT> & UniformGrid<ItemT>::operator=( const UniformGrid & that )
        {
            if( this != & that )
            {
                // Copy the base class (geometry) data
                UniformGridGeometry::operator=( that ) ;
                // Copy the actual grid contents
                mContents = that.mContents ;
            }
            return *this ;
        }
        // ---------------------------------------------------------------------



        /** Scale each value in this grid by the given scalar.

            \param scale - amount by which to scale each value in this grid

        */
           template<class ItemT>
        void UniformGrid<ItemT>::Scale( float scale )
        {
            PERF_BLOCK( UniformGrid__Scale ) ;

            const unsigned numCells = GetGridCapacity() ;
            for( unsigned offset = 0 ; offset < numCells ; ++ offset )
            {
                ItemT & rVal = (*this)[ offset ] ;
                rVal = rVal * scale;
            }
        }


        /** Initialize contents to whatever default ctor provides.
        */
       template<class ItemT>
        void UniformGrid<ItemT>::Init( const ItemT & initialValue )
        {
            PERF_BLOCK( UniformGrid__Init ) ;

            // First clear because Resize only assigns values to new items.
            mContents.Clear() ;
            mContents.Resize( GetGridCapacity() , initialValue ) ;
        }

    template<class ItemT>
        void UniformGrid<ItemT>::DefineShape( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 )
        {
            mContents.Clear() ;
            UniformGridGeometry::DefineShape( uNumElements , vMin , vMax , bPowerOf2 ) ;
        }


        /** Return the number of cells in this grid which have been assigned values.
        */
           template<class ItemT>
        size_t UniformGrid<ItemT>::Size() const { return mContents.Size() ; }


        /** Return whether this container contains any items.
        */
           template<class ItemT>
        bool UniformGrid<ItemT>::Empty() const {
            return mContents.Empty() ;
         }


        /** Create an empty container based on a given uniform grid container, but with lower resolution.

            \param src - Source UniformGridGeometry upon which to base dimensions of this container

            \param iDecimation - amount by which to reduce resolution (number of grid cells in each dimension).
                Typically this would be 2.

        */
           template<class ItemT>
        void UniformGrid<ItemT>::Decimate( const UniformGridGeometry & src , int iDecimation )
        {
            UniformGridGeometry::Decimate( src , iDecimation ) ;
        }

        //template<class ItemT>
        //void UniformGrid<ItemT>::UnitTest() ;


        /** Compute statistics of data in a uniform grid.

            \param min - minimum of all values in grid.  Caller must initialize to large values before calling this routine.

            \param max - maximum of all values in grid.  Caller must initialize to smale values before calling this routine.

        */
       template<class ItemT>
        void UniformGrid<ItemT>::ComputeStatistics( ItemT & min , ItemT & max ) const
        {
            PERF_BLOCK( UniformGrid__ComputeStatistics ) ;

            max = min = (*this)[ 0 ] ;
            const unsigned numCells = GetGridCapacity() ;
            for( unsigned offset = 0 ; offset < numCells ; ++ offset )
            {
                const ItemT & rVal = (*this)[ offset ] ;
                min = Min2( min , rVal ) ;
                max = Max2( max , rVal ) ;
            }
        }



        /** Interpolate values from grid to get value at given position.

            \param vResult      Interpolated value corresponding to value of grid contents at vPosition.

            \param vPosition    Position to sample.
        */
       template<class ItemT>
        void UniformGrid<ItemT>::Interpolate( ItemT & vResult , const Vec3 & vPosition ) const
        {
            unsigned long       indices[4] ; // Indices of grid cell containing position.
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = true ) ;
            IndicesOfPosition( indices , vPosition ) ;
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = false ) ;
            ASSERT( indices[0] < GetNumCells( 0 ) ) ;
            ASSERT( indices[1] < GetNumCells( 1 ) ) ;
            ASSERT( indices[2] < GetNumCells( 2 ) ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            const Vec3      vDiff         = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
            const Vec3      tween         = Vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ;
            const Vec3      oneMinusTween = Vec3( 1.0f , 1.0f , 1.0f ) - tween ;
            const size_t    numXY         = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
            const size_t    offsetX0Y0Z0  = OffsetFromIndices( indices ) ;
            const size_t    offsetX1Y0Z0  = offsetX0Y0Z0 + 1 ;
            const size_t    offsetX0Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
            const size_t    offsetX0Y0Z1  = offsetX0Y0Z0 + numXY ;
            const size_t    offsetX1Y0Z1  = offsetX0Y0Z0 + numXY + 1 ;
            const size_t    offsetX0Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
            vResult =     ( ( oneMinusTween.x * (*this)[ offsetX0Y0Z0 ]
                            +         tween.x * (*this)[ offsetX1Y0Z0 ] ) * oneMinusTween.y
                          + ( oneMinusTween.x * (*this)[ offsetX0Y1Z0 ]
                            +         tween.x * (*this)[ offsetX1Y1Z0 ] ) * tween.y        ) * oneMinusTween.z
                        + ( ( oneMinusTween.x * (*this)[ offsetX0Y0Z1 ]
                            +         tween.x * (*this)[ offsetX1Y0Z1 ] ) * oneMinusTween.y
                          + ( oneMinusTween.x * (*this)[ offsetX0Y1Z1 ]
                            +         tween.x * (*this)[ offsetX1Y1Z1 ] ) * tween.y        ) * tween.z ;
        }



        /** Interpolate values from grid to get value at given position.

            \param vResult      Interpolated value corresponding to value of grid contents at vPosition.

            \param vPosition    Position to sample.
        */
       template<class ItemT>
        void UniformGrid<ItemT>::InterpolateConditionally( ItemT & result , const Vec3 & vPosition ) const
        {
            unsigned long indices[4] ; // Indices of grid cell containing position.
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = true ) ;
            IndicesOfPosition( indices , vPosition ) ;
            DEBUG_ONLY( UniformGridGeometry::sInterpolating = false ) ;
            ASSERT( indices[0] < GetNumCells( 0 ) ) ;
            ASSERT( indices[1] < GetNumCells( 1 ) ) ;
            ASSERT( indices[2] < GetNumCells( 2 ) ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            const Vec3      vDiff         = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
            const Vec3      tween         = Vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ;
            const Vec3      oneMinusTween = Vec3( 1.0f , 1.0f , 1.0f ) - tween ;
            const size_t    numXY         = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
            const size_t    offsetX0Y0Z0  = OffsetFromIndices( indices ) ;
            const size_t    offsetX1Y0Z0  = offsetX0Y0Z0 + 1 ;
            const size_t    offsetX0Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
            const size_t    offsetX0Y0Z1  = offsetX0Y0Z0 + numXY ;
            const size_t    offsetX1Y0Z1  = offsetX0Y0Z0 + numXY + 1 ;
            const size_t    offsetX0Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
            float weightSum = 0.0f ;
            if( ! IsNan( (*this)[ offsetX0Y0Z0 ] ) ) { result += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * (*this)[ offsetX0Y0Z0 ] ; weightSum += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z ; }
            if( ! IsNan( (*this)[ offsetX1Y0Z0 ] ) ) { result +=         tween.x * oneMinusTween.y * oneMinusTween.z * (*this)[ offsetX1Y0Z0 ] ; weightSum +=         tween.x * oneMinusTween.y * oneMinusTween.z ; }
            if( ! IsNan( (*this)[ offsetX0Y1Z0 ] ) ) { result += oneMinusTween.x *         tween.y * oneMinusTween.z * (*this)[ offsetX0Y1Z0 ] ; weightSum += oneMinusTween.x *         tween.y * oneMinusTween.z ; }
            if( ! IsNan( (*this)[ offsetX1Y1Z0 ] ) ) { result +=         tween.x *         tween.y * oneMinusTween.z * (*this)[ offsetX1Y1Z0 ] ; weightSum +=         tween.x *         tween.y * oneMinusTween.z ; }
            if( ! IsNan( (*this)[ offsetX0Y0Z1 ] ) ) { result += oneMinusTween.x * oneMinusTween.y *         tween.z * (*this)[ offsetX0Y0Z1 ] ; weightSum += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z ; }
            if( ! IsNan( (*this)[ offsetX1Y0Z1 ] ) ) { result +=         tween.x * oneMinusTween.y *         tween.z * (*this)[ offsetX1Y0Z1 ] ; weightSum +=         tween.x * oneMinusTween.y *         tween.z ; }
            if( ! IsNan( (*this)[ offsetX0Y1Z1 ] ) ) { result += oneMinusTween.x *         tween.y *         tween.z * (*this)[ offsetX0Y1Z1 ] ; weightSum += oneMinusTween.x *         tween.y *         tween.z ; }
            if( ! IsNan( (*this)[ offsetX1Y1Z1 ] ) ) { result +=         tween.x *         tween.y *         tween.z * (*this)[ offsetX1Y1Z1 ] ; weightSum +=         tween.x *         tween.y *         tween.z ; }
            if( 0.0f == weightSum )
            {
                (float&) result = UNIFORM_GRID_INVALID_VALUE ;
            }
            else
            {
                ASSERT( ( weightSum > 0.0f ) && ( weightSum <= 1.0f ) ) ;
                result /= weightSum ;
                ASSERT( ! IsNan( result ) && ! IsInf( result ) ) ;
            }
        }



 



        /** Accumulate given value into grid at given position.

            \param vPosition - position of a "source" whose contents this routine stores in a grid cell.

            \param item - value of "source" to store into the grid cell that contains vPosition.

            \note "Accumulate" accumulates values; it does not overwrite them.
                    This routine does not populate the grid according to a function whose values
                    are given by "item".  Instead, this routine treats each insertion as though
                    the cell contains a specified "source", in addition to other sources that
                    might have already been inserted.

            \note Since this routine accumulates values, it is likely prudent to initialize
                    the values of each gridpoints to zero before calling this routine.

        */
       template<class ItemT>
        void UniformGrid<ItemT>::Accumulate( const Vec3 & vPosition , const ItemT & item )
        {
            unsigned  long      indices[4] ; // Indices of grid cell containing position.
            IndicesOfPosition( indices , vPosition ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            //ASSERT( vPosition >= vMinCorner ) ;
            const size_t    offsetX0Y0Z0  = OffsetFromIndices( indices ) ;
            const Vec3      vDiff         = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
            const Vec3      tween         = Clamp0to1( Vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ) ;
            const Vec3      oneMinusTween = Vec3( 1.0f , 1.0f , 1.0f ) - tween ;
            //if(     ( tween.x         < 0.0f ) || ( tween.y         < 0.0f ) || ( tween.z         < 0.0f )
            //    ||  ( oneMinusTween.x < 0.0f ) || ( oneMinusTween.y < 0.0f ) || ( oneMinusTween.z < 0.0f )
            //    ||  ( tween.x         > 1.0f ) || ( tween.y         > 1.0f ) || ( tween.z         > 1.0f )
            //    ||  ( oneMinusTween.x > 1.0f ) || ( oneMinusTween.y > 1.0f ) || ( oneMinusTween.z > 1.0f ) )
            //{
            //    printf( "position= %.15g %.15g %.15g   vMinCorner= %.15g %.15g %.15g\n", vPosition.x , vPosition.y , vPosition.z , vMinCorner.x , vMinCorner.y , vMinCorner.z ) ;
            //    printf( "diff= %g %g %g\n", vDiff.x , vDiff.y , vDiff.z ) ;
            //    printf( "tween= %g %g %g   oneMinusTween= %g %g %g\n", tween.x , tween.y , tween.z , oneMinusTween.x , oneMinusTween.y , oneMinusTween.z ) ;
            //    DEBUG_BREAK() ;
            //}
            //ASSERT( ( tween.x         <= 1.0f ) && ( tween.y         <= 1.0f ) && ( tween.z         <= 1.0f ) ) ;
            //ASSERT( ( tween.x         >= 0.0f ) && ( tween.y         >= 0.0f ) && ( tween.z         >= 0.0f ) ) ;
            //ASSERT( ( oneMinusTween.x <= 1.0f ) && ( oneMinusTween.y <= 1.0f ) && ( oneMinusTween.z <= 1.0f ) ) ;
            //ASSERT( ( oneMinusTween.x >= 0.0f ) && ( oneMinusTween.y >= 0.0f ) && ( oneMinusTween.z >= 0.0f ) ) ;
            const size_t    numXY         = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
            const size_t    offsetX1Y0Z0  = offsetX0Y0Z0 + 1 ;
            const size_t    offsetX0Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
            const size_t    offsetX0Y0Z1  = offsetX0Y0Z0 + numXY ;
            const size_t    offsetX1Y0Z1  = offsetX0Y0Z0 + numXY + 1 ;
            const size_t    offsetX0Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
            (*this)[ offsetX0Y0Z0 ] += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * item ;
            (*this)[ offsetX1Y0Z0 ] +=         tween.x * oneMinusTween.y * oneMinusTween.z * item ;
            (*this)[ offsetX0Y1Z0 ] += oneMinusTween.x *         tween.y * oneMinusTween.z * item ;
            (*this)[ offsetX1Y1Z0 ] +=         tween.x *         tween.y * oneMinusTween.z * item ;
            (*this)[ offsetX0Y0Z1 ] += oneMinusTween.x * oneMinusTween.y *         tween.z * item ;
            (*this)[ offsetX1Y0Z1 ] +=         tween.x * oneMinusTween.y *         tween.z * item ;
            (*this)[ offsetX0Y1Z1 ] += oneMinusTween.x *         tween.y *         tween.z * item ;
            (*this)[ offsetX1Y1Z1 ] +=         tween.x *         tween.y *         tween.z * item ;
        #if defined( _DEBUG ) && 0
            {
                const float weightSum = oneMinusTween.x * oneMinusTween.y * oneMinusTween.z +
                                                tween.x * oneMinusTween.y * oneMinusTween.z +
                                        oneMinusTween.x *         tween.y * oneMinusTween.z +
                                                tween.x *         tween.y * oneMinusTween.z +
                                        oneMinusTween.x * oneMinusTween.y *         tween.z +
                                                tween.x * oneMinusTween.y *         tween.z +
                                        oneMinusTween.x *         tween.y *         tween.z +
                                                tween.x *         tween.y *         tween.z ;
                ASSERT( Math::Resembles( weightSum , 1.0f ) ) ;
                const ItemT itemSum = oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * item
                                    +         tween.x * oneMinusTween.y * oneMinusTween.z * item
                                    + oneMinusTween.x *         tween.y * oneMinusTween.z * item
                                    +         tween.x *         tween.y * oneMinusTween.z * item
                                    + oneMinusTween.x * oneMinusTween.y *         tween.z * item
                                    +         tween.x * oneMinusTween.y *         tween.z * item
                                    + oneMinusTween.x *         tween.y *         tween.z * item
                                    +         tween.x *         tween.y *         tween.z * item ;
                using namespace Math ;
                ASSERT( Resembles( itemSum , item ) ) ;
            }
        #endif
        }




        /** Set all points surrounding the given cell to the specified value.
        */
template <class ItemT>
        void UniformGrid<ItemT>::SetCell( const Vec3 & vPosition , const ItemT & item )
        {
            unsigned long       indices[3] ; // Indices of grid cell containing position.
            IndicesOfPosition( indices , vPosition ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            const unsigned  numXY        = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
            const unsigned  offsetX0Y0Z0 = OffsetFromIndices( indices ) ;
            const unsigned  offsetX1Y0Z0 = offsetX0Y0Z0 + 1 ;
            const unsigned  offsetX0Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) ;
            const unsigned  offsetX1Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
            const unsigned  offsetX0Y0Z1 = offsetX0Y0Z0 + numXY ;
            const unsigned  offsetX1Y0Z1 = offsetX0Y0Z0 + numXY + 1 ;
            const unsigned  offsetX0Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
            const unsigned  offsetX1Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
            (*this)[ offsetX0Y0Z0 ] = item ;
            (*this)[ offsetX1Y0Z0 ] = item ;
            (*this)[ offsetX0Y1Z0 ] = item ;
            (*this)[ offsetX1Y1Z0 ] = item ;
            (*this)[ offsetX0Y0Z1 ] = item ;
            (*this)[ offsetX1Y0Z1 ] = item ;
            (*this)[ offsetX0Y1Z1 ] = item ;
            (*this)[ offsetX1Y1Z1 ] = item ;
        }




        /** Remove given component from all points surrounding the given cell.
        */
       template<class ItemT>
        void UniformGrid<ItemT>::RemoveComponent( const Vec3 & vPosition , const ItemT & component )
        {
            size_t          indices[4] ; // Indices of grid cell containing position.
            IndicesOfPosition( indices , vPosition ) ;
            Vec3            vMinCorner ;
            PositionFromIndices( vMinCorner , indices ) ;
            const size_t    numXY        = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
            const size_t    offsetX0Y0Z0 = OffsetFromIndices( indices ) ;
            const size_t    offsetX1Y0Z0 = offsetX0Y0Z0 + 1 ;
            const size_t    offsetX0Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
            const size_t    offsetX0Y0Z1 = offsetX0Y0Z0 + numXY ;
            const size_t    offsetX1Y0Z1 = offsetX0Y0Z0 + numXY + 1 ;
            const size_t    offsetX0Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
            const size_t    offsetX1Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
            (*this)[ offsetX0Y0Z0 ] -= (*this)[ offsetX0Y0Z0 ] * component * component ;
            (*this)[ offsetX1Y0Z0 ] -= (*this)[ offsetX1Y0Z0 ] * component * component  ;
            (*this)[ offsetX0Y1Z0 ] -= (*this)[ offsetX0Y1Z0 ] * component * component  ;
            (*this)[ offsetX1Y1Z0 ] -= (*this)[ offsetX1Y1Z0 ] * component * component  ;
            (*this)[ offsetX0Y0Z1 ] -= (*this)[ offsetX0Y0Z1 ] * component * component  ;
            (*this)[ offsetX1Y0Z1 ] -= (*this)[ offsetX1Y0Z1 ] * component * component  ;
            (*this)[ offsetX0Y1Z1 ] -= (*this)[ offsetX0Y1Z1 ] * component * component  ;
            (*this)[ offsetX1Y1Z1 ] -= (*this)[ offsetX1Y1Z1 ] * component * component  ;
        }




        /** Thread-safe version of RemoveComponent.
        */
        //       template<class ItemT>
        // void UniformGrid<ItemT>::RemoveComponent_ThreadSafe( const Vec3 & vPosition , const ItemT & component )
        // {
        //     using namespace Math ;
        //     unsigned long indices[4] ; // Indices of grid cell containing position.
        //     IndicesOfPosition( indices , vPosition ) ;
        //     Vec3            vMinCorner ;
        //     PositionFromIndices( vMinCorner , indices ) ;
        //     const unsigned  numXY        = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
        //     const unsigned  offsetX0Y0Z0 = OffsetFromIndices( indices ) ;
        //     const unsigned  offsetX1Y0Z0 = offsetX0Y0Z0 + 1 ;
        //     const unsigned  offsetX0Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) ;
        //     const unsigned  offsetX1Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
        //     const unsigned  offsetX0Y0Z1 = offsetX0Y0Z0 + numXY ;
        //     const unsigned  offsetX1Y0Z1 = offsetX0Y0Z0 + numXY + 1 ;
        //     const unsigned  offsetX0Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
        //     const unsigned  offsetX1Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX0Y0Z0 ] , - (*this)[ offsetX0Y0Z0 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX1Y0Z0 ] , - (*this)[ offsetX1Y0Z0 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX0Y1Z0 ] , - (*this)[ offsetX0Y1Z0 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX1Y1Z0 ] , - (*this)[ offsetX1Y1Z0 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX0Y0Z1 ] , - (*this)[ offsetX0Y0Z1 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX1Y0Z1 ] , - (*this)[ offsetX1Y0Z1 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX0Y1Z1 ] , - (*this)[ offsetX0Y1Z1 ] * component * component ) ;
        //     Vec3_FetchAndAdd( (*this)[ offsetX1Y1Z1 ] , - (*this)[ offsetX1Y1Z1 ] * component * component ) ;
        // }




        /** Restrict values from a given high-resolution grid into this low-resolution grid.

            In the vernacular of multi-grid solvers, the "restrict" operation reduces
            the resolution of a grid, so this operation is tantamount to down-sampling.
            It contrasts with the "interpolate" operation, which up-samples.

            This routine assumes this loRes grid is empty and the given hiRes layer is populated.

            \param hiRes - hiRes grid from which information will be aggregated.

            \note This loRes grid must have 3 or greater points in at least one of its dimensions.

            \see SolvePoissonMultiGrid UpSampleFrom

        */
       template<class ItemT>
        void UniformGrid<ItemT>::DownSampleSlice( const UniformGrid< ItemT > & hiRes , AccuracyVersusSpeedE accuracyVsSpeed , size_t izStart , size_t izEnd )
        {
            PERF_BLOCK( UniformGrid__DownSampleSlice ) ;

            ASSERT( hiRes.Size() == hiRes.GetGridCapacity() ) ;
            UniformGrid< ItemT > &  loRes        = * this ;
            ASSERT( ( loRes.GetNumPoints( 0 ) > 2 ) || ( loRes.GetNumPoints( 1 ) > 2 ) || ( loRes.GetNumPoints( 2 ) > 2 ) ) ;
            const unsigned  &       numXhiRes           = hiRes.GetNumPoints( 0 ) ;
            const unsigned          numXYhiRes          = numXhiRes * hiRes.GetNumPoints( 1 ) ;
            static const float      fMultiplierTable[]  = { 8.0f , 4.0f , 2.0f , 1.0f } ;

            // number of cells in each grid cluster
            const unsigned long pClusterDims[] = {   hiRes.GetNumCells( 0 ) / loRes.GetNumCells( 0 )
                                            ,   hiRes.GetNumCells( 1 ) / loRes.GetNumCells( 1 )
                                            ,   hiRes.GetNumCells( 2 ) / loRes.GetNumCells( 2 ) } ;
            ASSERT( pClusterDims[0] > 1 ) ;
            ASSERT( pClusterDims[0] > 1 ) ;
            ASSERT( pClusterDims[0] > 1 ) ;

            const unsigned  numPointsLoRes[3]   = { loRes.GetNumPoints( 0 ) , loRes.GetNumPoints( 1 ) , loRes.GetNumPoints( 2 ) } ;
            const unsigned  numXYLoRes          = loRes.GetNumPoints( 0 ) * loRes.GetNumPoints( 1 ) ;

            const unsigned          numPointsHiRes[3]   = { hiRes.GetNumPoints( 0 ) , hiRes.GetNumPoints( 1 ) , hiRes.GetNumPoints( 2 ) } ;
#       if USE_ALL_NEIGHBORS
            const unsigned long idxShifts[3]        = { pClusterDims[0] / 2 , pClusterDims[1] / 2 , pClusterDims[2] / 2 } ;
#       endif

            // Since this loop iterates over each destination cell, it parallelizes without contention.
            //
            // Note that the if-statements inside this loop could (and perhaps should) be
            // moved outside the loop.  Accomplish this by limiting the loRes indices to
            // be in [1,N-2] and then creating 6 2D loops below, for the boundary planes.
            unsigned idxLoRes[3] ;

            for( idxLoRes[2] = unsigned( izStart ) ; idxLoRes[2] < unsigned( izEnd ) ; ++ idxLoRes[2] )
            {
                const unsigned offsetLoZ = idxLoRes[2] * numXYLoRes ;
                for( idxLoRes[1] = 0 ; idxLoRes[1] < numPointsLoRes[1] ; ++ idxLoRes[1] )
                {
                    const unsigned offsetLoYZ = idxLoRes[1] * loRes.GetNumPoints( 0 ) + offsetLoZ ;
                    for( idxLoRes[0] = 0 ; idxLoRes[0] < numPointsLoRes[0] ; ++ idxLoRes[0] )
                    {   // For each cell in the loRes layer...
                        const unsigned  offsetLoXYZ   = idxLoRes[0] + offsetLoYZ ;
                        ItemT        &  rValLoRes  = loRes[ offsetLoXYZ ] ;
                        unsigned clusterMinIndices[ 3 ] ;
                        unsigned idxHiRes[3] ;


                        if( UniformGridGeometry::FASTER_LESS_ACCURATE == accuracyVsSpeed )
                        {
                            memset( & rValLoRes , 0 , sizeof( rValLoRes ) ) ;
                            //rValLoRes = ItemT{};
                            //NestedGrid<ItemT>::GetChildClusterMinCornerIndex( clusterMinIndices , pClusterDims , idxLoRes ) ;
                            GetChildClusterMinCornerIndexS( clusterMinIndices , pClusterDims , idxLoRes ) ;
                            idxHiRes[2] = clusterMinIndices[2] ;
                            idxHiRes[1] = clusterMinIndices[1] ;
                            idxHiRes[0] = clusterMinIndices[0] ;
                            const unsigned offsetZ      = idxHiRes[2] * numXYhiRes ;
                            const unsigned offsetYZ     = idxHiRes[1] * numXhiRes + offsetZ ;
                            const unsigned offsetXYZ    = idxHiRes[0] + offsetYZ ;
                            const ItemT &  rValHiRes    = hiRes[ offsetXYZ ] ;
                            rValLoRes = rValHiRes ;
                        }
                        else
                        {
  
                            ASSERT( UniformGridGeometry::SLOWER_MORE_ACCURATE == accuracyVsSpeed ) ;
                            float           multiplier  = 0.0f ;

                            //NestedGrid<ItemT>::GetChildClusterMinCornerIndex( clusterMinIndices , pClusterDims , idxLoRes ) ;
                            GetChildClusterMinCornerIndexS( clusterMinIndices , pClusterDims , idxLoRes ) ;
                            unsigned increment[3] ;
                            int      shiftHiRes[3] ;

                            // Code below accumulates values so destination needs to start at zero.
                            memset( & rValLoRes , 0 , sizeof( rValLoRes ) ) ;
                            //rValLoRes = ItemT{};

                            // For each cell of hiRes layer in this grid cluster...
                            for( increment[2] = 0 ; increment[2] < pClusterDims[2] IF_USE_ALL_NEIGHBORS( + idxShifts[2] ) ; ++ increment[2] )
                            {
                                shiftHiRes[2] = increment[2] IF_USE_ALL_NEIGHBORS( - idxShifts[2] ) ;
                                idxHiRes[2] = clusterMinIndices[2] + shiftHiRes[2] ;
                                if( idxHiRes[2] < numPointsHiRes[2] )
                                {
                                    const unsigned offsetZ  = idxHiRes[2] * numXYhiRes ;
                                    for( increment[1] = 0 ; increment[1] < pClusterDims[1] IF_USE_ALL_NEIGHBORS( + idxShifts[1] ) ; ++ increment[1] )
                                    {
                                        shiftHiRes[1] = increment[1] IF_USE_ALL_NEIGHBORS( - idxShifts[1] ) ;
                                        idxHiRes[1] = clusterMinIndices[1] + shiftHiRes[1] ;
                                        if( idxHiRes[1] < numPointsHiRes[1] )
                                        {
                                            const unsigned offsetYZ = idxHiRes[1] * numXhiRes + offsetZ ;
                                            for( increment[0] = 0 ; increment[0] < pClusterDims[0] IF_USE_ALL_NEIGHBORS( + idxShifts[0] ) ; ++ increment[0] )
                                            {
                                                shiftHiRes[0] = increment[0] IF_USE_ALL_NEIGHBORS( - idxShifts[0] ) ;
                                                idxHiRes[0] = clusterMinIndices[0] + shiftHiRes[0] ;
                                                if( idxHiRes[0]  < numPointsHiRes[0] )
                                                {
                                                    const unsigned  offsetXYZ       = idxHiRes[0]  + offsetYZ ;
                                                    const unsigned  manhattanDist   = ABS( shiftHiRes[0] ) + ABS( shiftHiRes[1] ) + ABS( shiftHiRes[2] ) ;
                                                    ASSERT( ( manhattanDist >= 0 ) && ( manhattanDist < 4 ) ) ;
                                                    const ItemT &   rValHiRes       = hiRes[ offsetXYZ ] ;
                                                    ASSERT( ! IsInf( rValHiRes ) && ! IsNan( rValHiRes ) ) ;
#                                               if USE_ALL_NEIGHBORS
                                                    multiplier += fMultiplierTable[ manhattanDist ] ;
                                                    rValLoRes  += fMultiplierTable[ manhattanDist ] * rValHiRes ;
                                                    ASSERT( multiplier <= 64.0 ) ;
#                                               else
                                                    multiplier += 1.0f ;
                                                    rValLoRes  += rValHiRes ;
#                                               endif
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Normalize sum to account for number of gridpoints that contributed to this gridpoint.
                            ASSERT( multiplier > 0.0f ) ;
//#                       if USE_ALL_NEIGHBORS
//                            rValLoRes /= 64.0f ;
//#                       else
                            rValLoRes /= multiplier ;
//#                       endif
                        }
                    }
                }
            }


#           if defined( _DEBUG )
if( UniformGridGeometry::SLOWER_MORE_ACCURATE == accuracyVsSpeed )
            {
                // Replaced the inaccurate Sum() approximation with the true spatial integral
                const ItemT zerothMomentLoRes = loRes.ComputeZerothMoment() ;
                const ItemT zerothMomentHiRes = hiRes.ComputeZerothMoment() ;
                
                ASSERT( Math::RResembles( zerothMomentLoRes, zerothMomentHiRes , 0.6f ) ) ;
            }
#           endif
        }



        template<class ItemT>
        void UniformGrid<ItemT>::DownSample( const UniformGrid & hiResSrc , AccuracyVersusSpeedE accuracyVsSpeed )
        {
            PERF_BLOCK( UniformGrid__DownSample ) ;

            const size_t numZ = GetNumPoints( 2 ) ;

#       if USE_TBB
            {
                // Estimate grain size based on size of problem and number of processors.
                const size_t grainSize =  Max2( size_t( 1 ) , numZ / gNumberOfProcessors ) ;
                parallel_for( tbb::blocked_range<size_t>( 0 , numZ , grainSize ) , UniformGrid_DownSample_TBB( * this , hiResSrc , accuracyVsSpeed ) ) ;
            }
#       else
            DownSampleSlice( hiResSrc , accuracyVsSpeed , 0 , numZ ) ;
#       endif
        }




        /** Interpolate value from the given low-resolution grid into this high-resolution grid.

            In the vernacular of multi-grid solvers, the "interpolate" operation increases
            the resolution of a grid, so this operation is tantamount to up-sampling.
            It contrasts with the "restrict" operation, which down-samples.

            This routine assumes the given high-resolution grid (this)
            is empty and the given loRes grid is populated.

            \param loRes - low-resolution grid from which information will be read.

            \see SolvePoissonMultiGrid DownSample

            \note As of 2009nov19, this routine does NOT properly "undo" the operations of DownSampleInto.
                    In multi-grid vernacular, this operation does NOT apply the transpose
                    of the Restriction operation.  This is evident by virtue of the fact
                    that after this operation, zeroth moment between the low-res and high-res
                    grids DO NOT MATCH.
                    A proper implementation would depend on the USE_ALL_NEIGHBORS flag,
                    and would preserve zeroth moment.
                    When USE_ALL_NEIGHBORS is DISabled, then this routine could simply
                    copy values from the low-res to high-res grids, without interpolation.
                    When USE_ALL_NEIGHBORS is ENabled, then this routine would need to use
                    a different interpolation algorithm -- not tri-linear, but something like
                    quadratic, where values come from all 27 neighbors of a position.

        */
       template<class ItemT>
        void UniformGrid<ItemT>::UpSampleSlice( const UniformGrid< ItemT > & loRes , UniformGridGeometry::RegionE region , size_t izStart , size_t izEnd )
        {
            PERF_BLOCK( UniformGrid__UpSampleSlice ) ;

            UniformGrid< ItemT > &  hiRes         = * this ;
            ASSERT( loRes.Size() == loRes.GetGridCapacity() ) ;
            const unsigned          numPointsHiRes[3]   = { hiRes.GetNumPoints( 0 ) , hiRes.GetNumPoints( 1 ) , hiRes.GetNumPoints( 2 ) } ;
            const unsigned  &       numXhiRes           = hiRes.GetNumPoints( 0 ) ;
            const unsigned          numXYhiRes          = numXhiRes * hiRes.GetNumPoints( 1 ) ;
            // Nudge sampling positions so they lie strictly within the grid.
            const Vec3              vPosMinAdjust       = Vec3( fabsf( hiRes.GetMinCorner().x )
                                                              , fabsf( hiRes.GetMinCorner().y )
                                                              , fabsf( hiRes.GetMinCorner().z ) ) * 2.0f * FLT_EPSILON ;
            const Vec3              vPosMin             = hiRes.GetMinCorner() + vPosMinAdjust ;
            ASSERT( ( hiRes.GetCellSpacing().x >= 0.0f ) && ( hiRes.GetCellSpacing().y >= 0.0f ) && ( hiRes.GetCellSpacing().z >= 0.0f ) ) ;
            const Vec3              vSpacing            = hiRes.GetCellSpacing() * ( 1.0f - 4.0f * FLT_EPSILON ) ;

            unsigned                idxMin[ 3 ]         = { 0 , 0 , unsigned( izStart ) } ;
            unsigned                idxMax[ 3 ]         = { numPointsHiRes[0] , numPointsHiRes[1] , unsigned( izEnd ) } ;

            if( UniformGridGeometry::INTERIOR_ONLY == region )
            {   // Up-sample only into interior gridpoint -- omit writing to boundary values.
                idxMin[0] = 1 ;
                idxMin[1] = 1 ;
                idxMin[2] = MAX2( 1U , unsigned( izStart ) ) ;

                idxMax[0] = MAX2( 1U , numPointsHiRes[0] ) - 1 ;
                idxMax[1] = MAX2( 1U , numPointsHiRes[1] ) - 1 ;
                idxMax[2] = MAX2( 1U , unsigned( izEnd ) ) - 1 ;
            }

            // Since this loop iterates over each destination cell, it parallelizes without contention.
            unsigned idxHiRes[3] ;
            for( idxHiRes[2] = idxMin[2] ; idxHiRes[2] < idxMax[2] ; ++ idxHiRes[2] )
            {
                const unsigned  offsetZ     = idxHiRes[2] * numXYhiRes ;
                Vec3            vPosition   ;
                vPosition.z = vPosMin.z + float( idxHiRes[2] ) * vSpacing.z ;

                for( idxHiRes[1] = idxMin[1] ; idxHiRes[1] < idxMax[1] ; ++ idxHiRes[1] )
                {
                    const unsigned offsetYZ = idxHiRes[1] * hiRes.GetNumPoints( 0 ) + offsetZ ;
                    vPosition.y = vPosMin.y + float( idxHiRes[1] ) * vSpacing.y ;

                    for( idxHiRes[0] = idxMin[0] ; idxHiRes[0] < idxMax[0] ; ++ idxHiRes[0] )
                    {   // For each cell in the loRes layer...
                        const unsigned  offsetXYZ   = idxHiRes[0] + offsetYZ ;
                        ItemT        &  rValHiRes   = hiRes[ offsetXYZ ] ;
                        vPosition.x = vPosMin.x + float( idxHiRes[0] ) * vSpacing.x ;
                        loRes.Interpolate( rValHiRes , vPosition ) ;
                        ASSERT( ! IsNan( rValHiRes ) && ! IsInf( rValHiRes ) ) ;
                    }
                }
            }

#       if defined( _DEBUG )
            if( UniformGridGeometry::ENTIRE_DOMAIN == region )
            {
                const ItemT zerothMomentLoRes = loRes.ComputeZerothMoment() ;
                const ItemT zerothMomentHiRes = hiRes.ComputeZerothMoment() ;
                ASSERT( Math::RResembles( zerothMomentLoRes, zerothMomentHiRes , 1.0e-2f ) ) ;
            }
#       endif
        }

        template<class ItemT>
        void UniformGrid<ItemT>::UpSample( const UniformGrid< ItemT > & loResSrc , UniformGridGeometry::RegionE region )
        {
            PERF_BLOCK( UniformGrid__UpSample ) ;

            const size_t numZ = GetNumPoints( 2 ) ;

#       if USE_TBB
            {
                // Estimate grain size based on size of problem and number of processors.
                const size_t grainSize =  Max2( size_t( 1 ) , numZ / gNumberOfProcessors ) ;
                parallel_for( tbb::blocked_range<size_t>( 0 , numZ , grainSize ) , UniformGrid_UpSample_TBB( * this , loResSrc , region ) ) ;
            }
#       else
            UpSampleSlice( loResSrc , region , 0 , numZ ) ;
#       endif
        }

        template<class ItemT>
        void UniformGrid<ItemT>::Clear()
        {
            //PERF_BLOCK( UniformGrid__Clear ) ;

            mContents.Clear() ;
            Parent::Clear() ;
        }

        template<>
        void UniformGrid<class ItemT>::GenerateBrickOfBytes( const char * strFilenameBase , unsigned uFrame ) const ;


        /** Compute sum of contents of this container.
        */
        template<class ItemT>
       ItemT UniformGrid<ItemT>:: Sum() const
        {
            ItemT     sum ;
            memset( & sum , 0 , sizeof( sum ) ) ;
            for( unsigned offsetP = 0 ; offsetP < GetGridCapacity() ; ++ offsetP )
            {
                sum += operator[]( offsetP ) ;
            }
            return sum ;
        }


        /** Compute zeroth moment of contents of this container.
        */
       template<class ItemT>
        ItemT UniformGrid<ItemT>::ComputeZerothMoment() const
        {
            PERF_BLOCK( UniformGrid__ComputeZerothMoment ) ;

            const unsigned long numCells[ 3 ] = { GetNumCells( 0 ) , GetNumCells( 1 ) , GetNumCells( 2 ) } ;
            const unsigned strides[ 3 ] = { 1 , GetNumPoints( 0 ) , GetNumPoints( 0 ) * GetNumPoints( 1 ) } ;
            ItemT     firstMoment ;
            memset( & firstMoment , 0 , sizeof( firstMoment ) ) ;
            unsigned idxCell[3] ;
            for( idxCell[2] = 0 ; idxCell[2] < numCells[ 2 ] ; ++ idxCell[2] )
            {
                for( idxCell[1] = 0 ; idxCell[1] < numCells[ 1 ] ; ++ idxCell[1] )
                {
                    for( idxCell[0] = 0 ; idxCell[0] < numCells[ 0 ] ; ++ idxCell[0] )
                    {   // For each cell...
                        const size_t offset = OffsetFromIndices( idxCell ) ;
                        firstMoment +=
                            (   operator[]( offset                                              )
                            +   operator[]( offset + strides[ 0 ]                               )
                            +   operator[]( offset +                strides[ 1 ]                )
                            +   operator[]( offset + strides[ 0 ] + strides[ 1 ]                )
                            +   operator[]( offset +                               strides[ 2 ] )
                            +   operator[]( offset + strides[ 0 ] +                strides[ 2 ] )
                            +   operator[]( offset +                strides[ 1 ] + strides[ 2 ] )
                            +   operator[]( offset + strides[ 0 ] + strides[ 1 ] + strides[ 2 ] )
                            ) ;



                    }
                }
            }
            static const float oneOverPointsPerCell = 0.125f ; // Each gridcell has 8 gridpoints.
            return firstMoment * GetCellVolume() * oneOverPointsPerCell ;
        }





template class UniformGrid<float>;
template class UniformGrid<int>;
template class UniformGrid<unsigned int>;
template class UniformGrid<unsigned long>;
template class UniformGrid<Vec3>;
template class UniformGrid<Mat33>;
template class UniformGrid<Vorton>;
template class UniformGrid<VortonClusterAux>;
template class UniformGrid< std::vector<unsigned int> >;

