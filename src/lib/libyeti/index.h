#ifndef yeti_index_h
#define yeti_index_h

#include <vector>

#include "class.h"
#include "tuple.h"

#include "index.hpp"
#include "permutation.hpp"
#include "tile.hpp"


#include "mallocimpl.h"
#include "yetiobject.h"

namespace yeti {

class IndexDescr : public smartptr::Serializable {
    
    private:
        /** An id for the the tile index, e.g, i for DOCC orbitals */
        std::string id_;

        /** A sentence-length description of the index*/
        std::string descr_;

        /** The number of tiles at a given level */
        size_t n_;

    public:
        /**
         * @param id See #id_
         * @param descr See #descr_
         * @param n See #n_
         */
        IndexDescr(
            const std::string& id,
            const std::string& descr,
            size_t n
        );

        virtual ~IndexDescr();

        /**
         * Whether two tile index descriptors describe the same index.
         * This only checks ID,
         * @param descr
         * @return .
         */
        bool equals(const constIndexDescrPtr& descr) const;

        uli n() const;

        std::string id() const;

        std::string descr() const;

};

class CompositeIndex :
    Malloc<CompositeIndex>
{
    private:
        char* compindex_;

        usi nindex_;

    public:
        CompositeIndex(
            const uli* indices,
            const usi* subset,
            usi nindex
        );
};

/**
 @class IndexRange
 Encapsulates a specific range of tile indices
*/
class IndexRange :
    public smartptr::Countable
{
    
    private:
        friend class TileIndexTest;
        
        /** The starting index for the tile*/
        uli start_;

        /** The number of indice in the tile.  This is the
         total number, not just the number of subindices. */
        uli n_;

        /**
            The index range number in the parent's list of subranges.
            Calling parent_->get(index_) should return this index range.
        */
        uli index_;

        IndexRange* parent_;

        /** A set of subindices. This is null for bottom level */
        SubindexTuplePtr subranges_;

        void validate();

        /** Given a set of subindex sizes, initialize the
            subindices */
        void init(const std::vector<uli>& subsizes);

        /** Given a total number of indices and the desired number
            of indices per tile, construct the set of sizes.  This
            is really only relevant for routines with irregular n.
            For example, if n is 13 and nper is 5, this would create
            blocks of size 5, 4, 4.
            @param n The total number of indices
            @param nper The requested number of indices per tile
            @param sizes Reference return
        */
        static void
        form_sizes(
            uli n,
            uli nper,
            std::vector<uli>& sizes
        );

        uli nsubranges(usi depth) const;

        void increment_offset(usi depth, uli offset);

    public:
        //static IndexRangeTuplePtr scalar_tuple;

        /**
         * Create a set of tile index as a composite of subindices
         * @param tuple
         * @parm idx The index number for the tile in its group of indices
         */
        IndexRange(
            uli start,
            const SubindexTuplePtr& tuple
        );

        IndexRange(
            const SubindexTuplePtr& tuple
        );

        /**
            Add a single extra metadata layer to the index range
        */
        IndexRange(
            IndexRange*,
            uli start = 0
        );

        /**
            Pad the index range with extra layers to match the given depth
        */
        IndexRange(
            IndexRange* range,
            usi depth,
            uli start = 0
        );

        /**
           Create an index range by picking out the subranges
           in the tuple.  For example, if start is 1 and nslice
           is 2, this would create an index range with subranges
           tuple[1] + tuple[2].
           @param start     The strarting index of the tuple slice
           @param nslice    The number of indices to include
           @param tuple     The tuple to base subranges on
           @param descr
         */
        IndexRange(
            uli start,
            uli nslice,
            const SubindexTuplePtr& tuple
        );

        /**
         Builds an index range with sliced subranges.  Based on nper,
         a set of subranges are sliced from tuple to create a subindex.
         For example, if tuple had 8 elements and nper was 3, this would
         create an index range with 3 subindices.  Subindex 0 would
         contain tuple[0]->tuple[2] as subindices, subindex 1 would contain
         tuple[3]->tuple[5], subindex 2 would have tuple[6]->tuple[7].
         @param start The starting index for the index range
         @param tuple The set of index ranges to build from
         @param descr
         @param nper  The desired number of indices per subindex
         */
        IndexRange(
            uli start,
            const SubindexTuplePtr& tuple,
            uli nper
        );

        IndexRange(
            uli start,
            uli n,
            IndexRange* subrange
        );

        /**
         * Create a new tile index with no subindices
         * @param start The index number start
         * @param n The number of indices in the range
         * @param descr A descriptor for the indices
         */
        IndexRange(
            uli start,
            uli n
         );

        /**
         * Create a new tile index with subindices of definite size
         * @param start The index number start
         * @param subsizes The sizes for each subindex
         * @param descr A descriptor for the indices
         */
        IndexRange(
            uli start,
            const std::vector<size_t>& subsizes
        );

        /**
         * Create a new tile index
         * @param start The index number start
         * @param n The number of indices in the range
         */
        IndexRange(
            uli start,
            uli n,
            uli nper
        );

        IndexRange(
            usi depth,
            uli nper,
            uli start = 0
        );

        virtual ~IndexRange();

        void increment_offsets();

        void increment_offsets(IndexRange* range);

        void set_offsets();

        void set_offsets(IndexRange* range);

        void set_parent(IndexRange* range);

        void get_subranges(std::list<IndexRange*>& sublist, usi depth);

        bool has_subrange(IndexRange* range);
        /**
            @return The depth the given subrange exists at
        */
        usi get_subdepth_alignment(IndexRange* range);

        static uli* get_zero_set();

        uli index() const;

        void set_index(uli index);

        /**
            @return The number of indices in the range
         */
        uli n() const;

        /**
            @param The depth to compute the total number of tiles at
            @return The total number of indices in the range at a given depth
        */
        uli ntot(usi depth = 0) const;

        /**
            @param The depth to compute the maximum size at
            @return The maximum range size at a given depth
        */
        uli nmax(usi depth = 0) const;

        /**
        */
        uli start(usi depth) const;

        /**
            @return The index (not inclusive) that defines the end of the index range.
                    Given three subranges (0-3), (6-8), (9-12) finish would be 13.
        */
        uli finish(usi depth) const;

        /**
            @return The index number the range begins on
         */
        uli start() const;

        usi depth() const;

        void expand_subrange_depth(usi maxdepth);

        /**
            @param idx The subindex
            @return The index range defining the subindex idx
         */
        IndexRange* get_subindex(size_t idx) const;

        IndexRange* get_parent() const;

        IndexRange* get_first_child() const;

        IndexRange* split_bottom_range() const;

        /**
            Create an index range in which
        */
        IndexRange* shift_bottom_range() const;

        void
        get_subindices(
            std::list<IndexRange*>& ranges,
            usi depth
        );

        /**
         * @return The set of index ranges for the subindices
         */
        SubindexTuplePtr get_subranges() const;

        /**
         * Create a subset index range for each index in the range
         * @param The size of each subrange
         */
        void split(size_t range);

        /** */
        void print(std::ostream& os = std::cout) const;

        /**
            Whether the two indices are equal
            @param idx
            @return
        */
        bool equals(IndexRange* idx) const;

        /**
            Figure out the number of bottom level index ranges
            with a given size.  Map key is the size and map value
            is the size.
        */
        void sizes(std::map<size_t, size_t>& sizes) const;

        /**
          * @return Whether the index range has subindices
          */
        bool is_parent() const;

};

class IndexRangeTuple :
    public YetiRuntimeCountable,
    public Malloc<IndexRangeTuple>
{

    private:
        usi nindex_;

        IndexRange** indices_;

        void recurse_get_index_tuples(
            usi index,
            IndexRange** ranges,
            std::list<IndexRangeTuplePtr>& tuples,
            usi depth
        );

    public:
        typedef IndexRange** iterator;

        IndexRangeTuple(usi size);

        IndexRangeTuple(usi size, IndexRange* tmpl);

        IndexRangeTuple(usi size, IndexRange** ranges);

        ~IndexRangeTuple();

        IndexRange** begin() const;

        IndexRangeTuple* copy(const PermutationPtr& p = 0) const;

        IndexRange** end() const;

        static IndexRangeTuple* get_unit_range(IndexRangeTuple* subrange);

        void get_index_tuples(
            std::list<IndexRangeTuplePtr>& tuples,
            usi depth
        );

        usi mindepth() const;

        usi maxdepth() const;

        bool is_aligned() const;

        void slice_front(usi nindex);

        void permute(Permutation* p);

        usi nindex() const;

        void set(usi idx, IndexRange* range);

        IndexRange* get(usi idx) const;

};

class IndexRangeLocation
    : public smartptr::Countable
{

    private:
        usi n_;

        uli* data_;

    public:
        IndexRangeLocation(const IndexRangeTuplePtr& tuple);

        bool lt(const IndexRangeLocationPtr& r);

        void print(std::ostream& os = std::cout) const;

        ~IndexRangeLocation();

};

class IndexRangeLocationCompare {

    public:
        bool operator()(const IndexRangeLocationPtr& l, const IndexRangeLocationPtr& r);

};

/**
 @class CompositeIndexer
 Takes a complete set of indices and compute an index for them
*/
class Indexer : public smartptr::Serializable, public Malloc<Indexer> {

    private:

        /** The total number of indices */
        usi nindex_;

        /** The total number of all possible index sets */
        uli nsets_;

        /** The array of cumulative total sizes.  If you have indices of size
            ni, nj, nk then the arr is narr = {nj * ni, nj, 1} */
        uli* cumulsizes_;

        /**
            The array of offsets for the indexer. For example, if
            offsets were {3, 0, 5}, the array {4, 2, 7} would be
            indexed as {1, 2, 2}.
        */
        uli* offsets_;

        /**
            Workspace array used for computing permutations to
            avoid excessive malloc calls. This should be the size of
            the full index set, not just the index subset.
        */
        uli* permuted_;

        /**
            Workspace array for grabbing desired indices from larger array
        */
        uli* tmpindices_;

        /**
            When passed a set of indices, grab these out of the complete set.
            For example, if indexmap were {0, 3, 4} and the index routine was
            passed {3, 1, 2, 5, 6} then {3, 5, 6} would get indexed.
        */
        usi* indexmap_;

        /**
         * Initialize a composite indexer for these index ranges
         * @param tuple The index ranges
         */
        void init(const IndexRangeTuplePtr& tuple);

        void init(
            const uli* sizes,
            const uli* offsets,
            const usi* indexmap
        );

    private:
        /**
            Constructor for making copies
        */
        Indexer(const Indexer* parent);

    public:
        /**
         * Constructor
         * @param tuple
         * @param pgrp  Permutation group defining equivalent quantities
         */
        Indexer(
            const IndexRangeTuplePtr& tuple
        );

        Indexer(
            const size_t* sizes,
            const size_t* offsets,
            const usi* indexmap,
            usi nindex,
            const PermutationPtr& p
        );

        Indexer(
            const size_t* sizes,
            const size_t* offsets,
            usi nindex,
            const PermutationPtr& p
        );

        /**
         * Constructor for subset
         * @param tuple
         * @param pgrp
         * @param indexmap The list of index ranges to get from tuple
         * @param nindex The number of indices to get
         */
        Indexer(
            const IndexRangeTuplePtr& tuple,
            const usi* indexmap,
            usi nindex,
            const PermutationPtr& p
        );

        Indexer();

        virtual ~Indexer();

        Indexer* copy() const;

        /**
         *
         * @param indices The index set to index
         * @return The composite index
         */
        uli index(const size_t* indices) const;

        uli index(const TilePtr& tile) const;

        /**
         * Given a composite index, extract the individual indices
         * @param index
         * @param indices
         */
        void extract(uli index, uli** indices) const;

        /**
         * @return The total number of indices in the composite index
         */
        usi nindex() const;

        /**
         * @return The total number of index sets
         */
        uli nsets() const;

        void print(std::ostream& os = std::cout) const;

        void permute(const PermutationPtr& p);

};

class SubindexTuple :
    public CountableArray<IndexRange>
{

    public:
        SubindexTuple(uli n, IndexRange* tmpl);

        SubindexTuple(uli n);

        SubindexTuplePtr slice(uli start, uli stop);

        /**
            @param The subindex to find the index number for
            @return The index of a given subindex
        */
        uli index(IndexRange* subidx) const;

};

std::ostream&
operator<<(std::ostream& os, IndexRange* range);

std::ostream&
operator<<(std::ostream& os, const IndexRangeTuplePtr& tuple);

std::ostream&
operator<<(std::ostream& os, IndexRangeTuple* tuple);

} //end namespace

#endif

