/** \file uniformGrid.h

    \brief A container for fast spatial lookups and insertions

    \author Copyright 2009-2016 Dr. Michael Jason Gourlay; All rights reserved.  Contact me at mijagourlay.com for licensing.
        - http://www.mijagourlay.com/
*/
#ifndef UNIFORM_GRID_H
#define UNIFORM_GRID_H

#include <Core/useTbb.h>

#include <Core/Performance/perfBlock.h>

#include <Core/Math/vec3.h>

#include <Core/Utility/macros.h>
#include <Core/File/debugPrint.h>
#include <Core/Containers/vector.h>
#include <Core/Math/math.h>

#if USE_TBB
#   include <tbb/atomic.h>
#endif

#include <algorithm>

#include <math.h>

#define USE_ALL_NEIGHBORS 1

//#define Max2(a,b) MAX2(a,b)

// Macros --------------------------------------------------------------

#define UNIFORM_GRID_INVALID_VALUE std::numeric_limits< float >::quiet_NaN()

/** Whether to use all neighbors when performing downsamping.

    \note   The "diffusing" version of this downsampler might be more appropriate
            when assigning values for the maximal gridpoints.

    Ultimately, the choice of which technique to use depends on which provides better results,
    including performance.  Remember that the results on the low-res grid are effectively only
    starters for the results obtained on the high-res grid, so to that extent even an approximate
    answer provides value.  So it's probably better to favor speed over accuracy here.
*/

#if USE_ALL_NEIGHBORS
    /// Apply expression since USE_ALL_NEIGHBORS is enabled.
#   define IF_USE_ALL_NEIGHBORS( expr ) expr
#else
#   error This case has not been tested in a while and has probably stopped working.
    /// Ignore expression since USE_ALL_NEIGHBORS is disabled.
#   define IF_USE_ALL_NEIGHBORS( expr )
#endif


/// Return the absolute value.
#define ABS( x ) ( ( (x) < 0 ) ? ( - (x) ) : (x) )

#if USE_TBB
namespace Math
{
    /// Atomically increment sum.
    inline void Float_FetchAndAdd( float & sum , const float & increment )
    {
        tbb::atomic<float> & atomicSum = reinterpret_cast< tbb::atomic<float> & >( sum );
        float sumOld , sumNew ;
        do
        {
            sumOld = atomicSum ;
            sumNew = sumOld + increment ;
        } while( atomicSum.compare_and_swap( sumNew , sumOld ) != sumOld ) ;
    }


    /// Atomically increment sum.
    inline void Vec3_FetchAndAdd( Vec3 & sum , const Vec3 & increment )
    {
        Float_FetchAndAdd( sum.x , increment.x ) ;
        Float_FetchAndAdd( sum.y , increment.y ) ;
        Float_FetchAndAdd( sum.z , increment.z ) ;
    }
}

#endif


// Types --------------------------------------------------------------

struct Stats_Float
{
    Stats_Float() : mMean( 0.0f ) , mStdDev( 0.0f ) , mMin( FLT_MAX ) , mMax( -FLT_MAX ) {}

    float   mMean   ;
    float   mStdDev ;
    float   mMin    ;
    float   mMax    ;
} ;




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
class UniformGridGeometry
{
    public:
        enum RegionE
        {
            ENTIRE_DOMAIN   ,   /// All gridpoints
            INTERIOR_ONLY   ,   /// Interior gridpoints, excluding outer-most layer (the border)
        } ;

        enum AccuracyVersusSpeedE
        {
            SLOWER_MORE_ACCURATE    ,
            FASTER_LESS_ACCURATE    ,
        } ;


        /** Construct an empty UniformGridGeometry.

            \see Clear
        */
        UniformGridGeometry();


        /** Construct a uniform grid that fits the given geometry.

            \see Clear, DefineShape
        */
        UniformGridGeometry( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 );

        /// Return minimal corner of this UniformGrid.
        const Vec3 & GetMinCorner() const;


        /// Return maximal corner of this UniformGrid.
        Vec3        GetMaxCorner() const ;


        /** Return whether the given query point lies inside the boundaries of this grid.
        */
        bool    Encompasses( const Vec3 & queryPoint ) const;


        /// Return center position that this UniformGrid contains.
        Vec3        GetCenter() const ;


        /** Return whether the shape of this object matches that of the given.

            \param that - Other UniformGridGeometry object

            \return true if the shape of this object matches that of that.

        */
        bool ShapeMatches( const UniformGridGeometry & that ) const;



        bool operator==( const UniformGridGeometry & that ) const ;




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
        void DefineShape( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 );
        

        /** Scale given grid.

            \param src - Source uniform grid upon which to base dimensions of this one

            \param scale - amount by which to scale each axis of grid

            \note The number of cells is also scaled.

        */
        void Scale( const UniformGridGeometry & src , const Vec3 & scale );



        /** Create a higher-resolution uniform grid based on another.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iExpansion  Amount by which to increase the number of grid cells in each dimension.
                Typically this would be a power of 2.

            \note The number of cells is increased.  The number of points is different.

        */
        void Expand( const UniformGridGeometry & src , int iExpansions[ 3 ] );




        /** Create a higher-resolution uniform grid based on another.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iExpansion  Amount by which to increase the number of grid cells in each dimension.
                Typically this would be a power of 2.

            \note The number of cells is increased.  The number of points is different.

        */
        void Expand( const UniformGridGeometry & src , int expansion );




        /** Make this a lower-resolution uniform grid based the given source grid.

            \param src  Source uniform grid upon which to base dimensions of this one.

            \param iDecimation  Amount by which to reduce the number of grid cells in each dimension.
                Typically this would be 2.

            \note The number of cells is decimated.  The number of points is different.

        */
        void Decimate( const UniformGridGeometry & src , int iDecimation );



        /** Copy shape information from another UniformGrid into this one.
        */
        void CopyShape( const UniformGridGeometry & src );




        /** Use shape information from another UniformGrid and contraints to determine shape of this grid.
        */
        void FitShape( const UniformGridGeometry & src , const float minCellSpacing );


        /** Get world-space dimensions of UniformGridGeometry.
        */
        const Vec3 & GetExtent() const;


        /** Whether this geometry has zero extent.
        */
        bool HasZeroExtent() const;


        /** Get reciprocal of cell spacing.
        */
        const Vec3 & GetCellsPerExtent() const;


        /** Get number of grid points along the given dimension.

            \param index - dimension queried, where 0 means x, 1 means y and 2 means z.

            \note The number of cells in each direction i is GetNumPoints(i) - 1.

        */
        const unsigned    GetNumPoints( const unsigned & index ) const;


        /** Get number of grid cells along the given dimension.

            \param index - dimension queried, where 0 means x, 1 means y and 2 means z.

            \see GetNumPoints

        */
        unsigned  long      GetNumCells( const unsigned & index ) const;


        /** Get total number of gridpoints.

            \note This returns the number of gridpoints defined by the geometry, which is separate
                    from the capacity of the container.  When the container has been initialized,
                    these should be the same, but prior to initializing the container,
                    GetGridCapacity can be non-zero even when the container size is zero.

        */
        unsigned GetGridCapacity() const;


        /** Return extent (in world units) of a grid cell.
        */
        const Vec3 &    GetCellSpacing() const;


        /** Return volume (in world units) of entire grid.
        */
        const float GetVolume() const ;


        /** Return volume (in world units) of a grid cell.
        */
        const float GetCellVolume() const;


        /** Return extent (in world units) of a grid cell.
        */
        const Vec3  GetCellCenter( int ix , int iy , int iz ) const;


        /** Compute indices into contents array of a point at a given position.

            \param vPosition - position of a point.  It must be within the region of this container.

            \param indices - Indices into contents array of a point at vPosition.

            \see IndicesFromOffset, PositionFromOffset, OffsetOfPosition, PositionFromIndices

            \note Derived class defines the actual contents array.

        */
        void IndicesOfPosition( unsigned long indices[4] , const Vec3 & vPosition ) const;


        /** Get indices of nearest grid point.

            \param indices      (out) Indices of nearest grid point.

            \param vPosition    (in) Position being queried.

            This contrasts with IndicesOfPosition, which gets the indices of the nearest grid *cell*,
            which is tantamount to getting the minimal corner gridpoint of that cell.
            IndicesOfPosition is like rounding down whereas this routine is like
            rounding to nearest.
        */
        void IndicesOfNearestGridPoint( unsigned long indices[4] , const Vec3 & vPosition ) const;


        /** Compute offset into contents array of a point at a given position.

            \param vPosition - position of a point.  It must be within the region of this container.

            \return Offset into contents array of a point at vPosition.

            \see IndicesFromOffset, PositionFromOffset, PositionFromIndices

            \note Derived class defines the actual contents array.

        */
        unsigned    OffsetOfPosition( const Vec3 & vPosition ) const;


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
        void    PositionFromIndices( Vec3 & vPosition , const unsigned long indices[3] ) const;


        /** Compute position of minimal corner of grid cell with given indices.

            \param ix   X index of grid cell.
            \param iy   Y index of grid cell.
            \param iz   Z index of grid cell.

            \return Position of minimal corner of grid cell.
        */
        Vec3    PositionFromIndices( unsigned long ix , unsigned long iy , unsigned long iz ) const;


        /** Compute position of minimal corner of grid cell with given indices.

            \param indices      (in) Grid cell indices.

            \return Position of minimal corner of grid cell.

        */
        Vec3    PositionFromIndices( const unsigned long indices[3] ) const;


        /** Compute X,Y,Z grid cell indices from offset into contents array.

            \param indices - Individual X,Y,Z component grid cell indices.

            \param offset - Offset into mContents.
        */
        void    IndicesFromOffset( unsigned long indices[3] , const unsigned long & offset ) const;


        /** Get position of grid cell minimum corner.

            \param vPos     Position of grid cell minimum corner.

            \param offset   Offset into contents array.

            Each grid cell spans a region (whose size is given by GetCellSpacing)
            starting at a location which this routine returns.  So the grid cell
            with the given offset spans the region from vPos (as this routine
            assigns) to vPos + GetCellSpacing().

            \note Derived class provides actual contents array.

        */
        void    PositionFromOffset( Vec3 & vPos , const unsigned long & offset );


        /** Get offset into contents array given indices.

            \param ix   X index of grid point.
            \param iy   Y index of grid point.
            \param iz   Z index of grid point.

            \return offset into contents array

        */
        size_t OffsetFromIndices( size_t ix , size_t iy , size_t iz ) const;


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
        size_t OffsetFromIndices( const size_t indices[4] ) const;

        size_t OffsetFromIndices( const unsigned int indices[4] ) const;


        /** Precompute grid spacing, to optimize OffsetOfPosition and other utility routines.
        */
        void PrecomputeSpacing();



        /** Return the offset associated with the cell whose indices are second-to-last in each direction.

            Same as nx * ( ny * ( nz - 1 ) - 1 ) - 2.
        */
        unsigned GetOffsetOfPenultimateCell();


        /** Clear out any existing shape information.
        */
        void Clear();


        Vec3    mMinCorner      ;   ///< Minimum position (in world units) of grid in X, Y and Z directions.
        Vec3    mGridExtent     ;   ///< Size (in world units) of grid in X, Y and Z directions.
        Vec3    mCellExtent     ;   ///< Size (in world units) of a cell.
        Vec3    mCellsPerExtent ;   ///< Reciprocal of cell size (precomputed once to avoid excess divides).
        size_t  mNumPoints[ 3 ] ;   ///< Number of gridpoints along X, Y and Z directions.

        DEBUG_ONLY( static bool sInterpolating ) ;  ///< Whether Interpolate is being called.
} ;



#if USE_TBB
    /** Templated function object to down-sample
    */
    class UniformGrid_DownSample_TBB
    {
                  UniformGrid &                         mLoResDst               ;   /// Reference to object into which to down-sample
            const UniformGrid &                         mHiResSrc               ;   /// Reference to object from which to down-sample
            UniformGridGeometry::AccuracyVersusSpeedE   mAccuracyVersusSpeed    ;   /// How to down-sample from finer grid
        public:
            void operator() ( const tbb::blocked_range<size_t> & r ) const;
     
            UniformGrid_DownSample_TBB( UniformGrid & loResDst , const UniformGrid & hiResSrc , UniformGridGeometry::AccuracyVersusSpeedE accuracyVsSpeed )
                : mLoResDst( loResDst )
                , mHiResSrc( hiResSrc )
                , mAccuracyVersusSpeed( accuracyVsSpeed );
        private:
            WORD        mMasterThreadFloatingPointControlWord   ;
            unsigned    mMasterThreadMmxControlStatusRegister   ;
    } ;


    /** Templated function object to up-sample
    */
    class UniformGrid_UpSample_TBB
    {
                  UniformGrid &             mHiResDst   ;   /// Reference to object into which to up-sample
            const UniformGrid &             mLoResSrc   ;   /// Reference to object from which to up-sample
            UniformGridGeometry::RegionE    mRegion     ;   /// Into what region to up-sample
        public:
            void operator() ( const tbb::blocked_range<size_t> & r ) const;
            UniformGrid_UpSample_TBB( UniformGrid & hiResDst , const UniformGrid & loResSrc , UniformGridGeometry::RegionE region )
                : mHiResDst( hiResDst )
                , mLoResSrc( loResSrc )
                , mRegion( region );
        private:
            WORD        mMasterThreadFloatingPointControlWord   ;
            unsigned    mMasterThreadMmxControlStatusRegister   ;
    } ;
#endif


/** Templated container for fast spatial lookups and insertions.
*/
template <class ItemT> class UniformGrid : public UniformGridGeometry
{

    public:

        typedef UniformGridGeometry Parent ;    ///< Nickname for UniformGridGeometry.

        /** Construct an empty UniformGrid.
            \see Initialize
        */
        UniformGrid();


        /** Construct a uniform grid container that fits the given geometry.
            \see Initialize
        */
        UniformGrid( unsigned uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 );


        /** Copy shape from given uniform grid.
        */
        explicit UniformGrid( const UniformGridGeometry & that );


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
        UniformGrid( const UniformGrid & that );

        UniformGrid & operator=( const UniformGrid & that );

        /// Return contents

#   if _MSC_VER && ( _MSC_VER < 1700 ) // Older compiler...
              ItemT *       Data()       { return /* mContents.Empty() ? 0 : */ & mContents[ 0 ] ; }
        const ItemT *       Data() const { return /* mContents.Empty() ? 0 : */ & mContents[ 0 ] ; }
#   else
              ItemT *       Data()       { return mContents.data() ; }
        const ItemT *       Data() const { return mContents.data() ; }
#   endif

        /// Return item at given offset, where offset should come from OffsetOfPosition or OffsetFromIndices.
              ItemT &       operator[]( const size_t & offset )       { ASSERT( offset < Size() ) ; return mContents[ offset ] ; }
        const ItemT &       operator[]( const size_t & offset ) const { ASSERT( offset < Size() ) ; return mContents[ offset ] ; }

        /// Return item at given indices.
              ItemT &       operator[]( const size_t indices[] )    { return mContents[ OffsetFromIndices( indices ) ] ; }
              ItemT &       operator[]( const unsigned int indices[] )    { return mContents[ OffsetFromIndices( indices ) ] ; }

        /// Return item at given indices.
              ItemT &       Get( size_t ix , size_t iy , size_t iz )       { return mContents[ OffsetFromIndices( ix , iy , iz ) ] ; }
        const ItemT &       Get( size_t ix , size_t iy , size_t iz ) const { return mContents[ OffsetFromIndices( ix , iy , iz ) ] ; }

        /// Return item at given position.
              ItemT &       operator[]( const Vec3 & vPosition )        { return mContents[ OffsetOfPosition( vPosition ) ] ; }
        const ItemT &       operator[]( const Vec3 & vPosition ) const  { return mContents[ OffsetOfPosition( vPosition ) ] ; }




        bool operator==( const UniformGrid & that ) const
        {
            PERF_BLOCK( UniformGrid__opIsEqual ) ;

            if( ! this->Parent::operator==( that ) )
            {   // Geometries do not match, so these grids are not equal.
                return false ;
            }
            const size_t numItems = Size() ;
            // Check contents for equality.
            for( size_t idx = 0 ; idx < numItems ; ++ idx )
            {   // For each item in this grid...
                if( (*this)[ idx ] != that[ idx ] )
                {
                    return false ;
                }
            }
            return true ;
        }




        /** Scale each value in this grid by the given scalar.

            \param scale - amount by which to scale each value in this grid

        */
        void Scale( float scale );


        /** Initialize contents to whatever default ctor provides.
        */
        void Init( const ItemT & initialValue = ItemT() );


        void DefineShape( size_t uNumElements , const Vec3 & vMin , const Vec3 & vMax , bool bPowerOf2 );


        /** Return the number of cells in this grid which have been assigned values.
        */
        size_t Size() const;


        /** Return whether this container contains any items.
        */
        bool Empty() const;


        /** Create an empty container based on a given uniform grid container, but with lower resolution.

            \param src - Source UniformGridGeometry upon which to base dimensions of this container

            \param iDecimation - amount by which to reduce resolution (number of grid cells in each dimension).
                Typically this would be 2.

        */
        void Decimate( const UniformGridGeometry & src , int iDecimation );


        static void UnitTest() ;


        /** Compute statistics of data in a uniform grid.

            \param min - minimum of all values in grid.  Caller must initialize to large values before calling this routine.

            \param max - maximum of all values in grid.  Caller must initialize to smale values before calling this routine.

        */
        void ComputeStatistics( ItemT & min , ItemT & max ) const;



        /** Interpolate values from grid to get value at given position.

            \param vResult      Interpolated value corresponding to value of grid contents at vPosition.

            \param vPosition    Position to sample.
        */
        void Interpolate( ItemT & vResult , const Vec3 & vPosition ) const;



        /** Interpolate values from grid to get value at given position.

            \param vResult      Interpolated value corresponding to value of grid contents at vPosition.

            \param vPosition    Position to sample.
        */
        void InterpolateConditionally( ItemT & result , const Vec3 & vPosition ) const;
        


 



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
        void Accumulate( const Vec3 & vPosition , const ItemT & item );




        /** Set all points surrounding the given cell to the specified value.
        */
        void SetCell( const Vec3 & vPosition , const ItemT & item );




        /** Remove given component from all points surrounding the given cell.
        */
        void RemoveComponent( const Vec3 & vPosition , const ItemT & component );




        /** Thread-safe version of RemoveComponent.
        */
        void RemoveComponent_ThreadSafe( const Vec3 & vPosition , const ItemT & component );




        /** Restrict values from a given high-resolution grid into this low-resolution grid.

            In the vernacular of multi-grid solvers, the "restrict" operation reduces
            the resolution of a grid, so this operation is tantamount to down-sampling.
            It contrasts with the "interpolate" operation, which up-samples.

            This routine assumes this loRes grid is empty and the given hiRes layer is populated.

            \param hiRes - hiRes grid from which information will be aggregated.

            \note This loRes grid must have 3 or greater points in at least one of its dimensions.

            \see SolvePoissonMultiGrid UpSampleFrom

        */
        void DownSampleSlice( const UniformGrid< ItemT > & hiRes , AccuracyVersusSpeedE accuracyVsSpeed , size_t izStart , size_t izEnd );
        




        void DownSample( const UniformGrid & hiResSrc , AccuracyVersusSpeedE accuracyVsSpeed );




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
        void UpSampleSlice( const UniformGrid< ItemT > & loRes , UniformGridGeometry::RegionE region , size_t izStart , size_t izEnd );
        


        void UpSample( const UniformGrid< ItemT > & loResSrc , UniformGridGeometry::RegionE region );


        void Clear();


        void GenerateBrickOfBytes( const char * strFilenameBase , unsigned uFrame ) const ;


        /** Compute sum of contents of this container.
        */
        ItemT Sum() const;


        /** Compute zeroth moment of contents of this container.
        */
        ItemT ComputeZerothMoment() const;


    private:
        VECTOR<ItemT>   mContents   ;   ///< 3D array of items.
} ;

// Public variables --------------------------------------------------------------
// Public functions --------------------------------------------------------------

#endif
