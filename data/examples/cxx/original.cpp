static const Node *
getParent(const Node *x)
{
    do {
        x = x->parent;
    } while (x != nullptr && isUnmovable(x));
    return x;
}
