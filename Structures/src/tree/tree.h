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
        NODE_FREE,
        NODE_USED,
        NODE_IMMUTABLE,
        NODE_PROPOSED,
    } state : 32;
};

struct node {
    node_branch left  : 64;
    node_branch right : 64;
    node_ref    ref   : 64;
};

struct tree_root {
    alloc *refs;
    alloc *nodes;

    node_branch root;
};
