const Node *
Comparator::getParent(const Node *x)
{
    do {
        x = x->parentNode;
    } while (x != nullptr &&
             lang.isUnmovable(x));
    return x;
}
