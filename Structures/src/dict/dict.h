struct node_ref {
    uint32_t ref_id;
    enum {
        NODE_LOCAL,
        NODE_SHARED,
        NODE_MOVED,
        NODE_REMOVED,
    } state : 32;
};

struct node_branch {
    uint32_t node_id;
    enum {
        NODE_OPEN,
        NODE_IMMUTABLE,
        NODE_REBALANCE,
        NODE_REBUILD
    } state : 32;
};

struct node {
    node_branch left  : 64;
    node_branch right : 64;
    node_ref    ref   : 64;
};

struct dict_base {
    alloc *nodes;
    alloc *refs;

    uint32_t     root_count;
    node_branch *roots;
};

struct dict {
    singularity *s;
    dict_base *current;
    dict_base *building;
};


