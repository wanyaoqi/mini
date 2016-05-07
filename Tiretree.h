#include <iostream>
using namespace std;

/* trie的节点类型 */
template <int Size> //Size为字符表的大小
struct trie_node
{
	bool terminable; //当前节点是否可以作为字符串的结尾
	int node; //子节点的个数
	trie_node *child[Size]; //指向子节点指针

	/* 构造函数 */
	trie_node() : terminable(false), node(0) { memset(child, 0, sizeof(child)); }
};

/* trie */
template <int Size, typename Index> //Size为字符表的大小，Index为字符表的哈希函数
class trie
{
public:
	/* 定义类型别名 */
	typedef trie_node<Size> node_type;
	typedef trie_node<Size>* link_type;

	/* 构造函数 */
	trie(Index i = Index()) : index(i){ }

	/* 析构函数 */
	~trie() { clear(); }

	/* 清空 */
	void clear()
	{
		clear_node(root);
		for (int i = 0; i < Size; ++i)
			root.child[i] = 0;
	}

	/* 插入字符串 */
	template <typename Iterator>
	void insert(Iterator begin, Iterator end)
	{
		link_type cur = &root; //当前节点设置为根节点
		for (; begin != end; ++begin)
		{
			if (!cur->child[index[*begin]]) //若当前字符找不到匹配，则新建节点
			{
				cur->child[index[*begin]] = new node_type;
				++cur->node; //当前节点的子节点数加一
			}
			cur = cur->child[index[*begin]]; //将当前节点设置为当前字符对应的子节点
		}
		cur->terminable = true; //设置存放最后一个字符的节点的可终止标志为真
	}

	/* 插入字符串，针对C风格字符串的重载版本 */
	void insert(const char *str)
	{
		insert(str, str + strlen(str));
	}

	/* 查找字符串，算法和插入类似 */
	template <typename Iterator>
	bool find(Iterator begin, Iterator end)
	{
		link_type cur = &root;
		for (; begin != end; ++begin)
		{
			if (!cur->child[index[*begin]])
				return false;
			cur = cur->child[index[*begin]];
		}
		return cur->terminable;
	}

	/* 查找字符串，针对C风格字符串的重载版本 */
	bool find(const char *str)
	{
		return find(str, str + strlen(str));
	}

	/* 删除字符串 */
	template <typename Iterator>
	bool erase(Iterator begin, Iterator end)
	{
		bool result; //用于存放搜索结果
		erase_node(begin, end, root, result);
		return result;
	}

	/* 删除字符串，针对C风格字符串的重载版本 */
	bool erase(char *str)
	{
		return erase(str, str + strlen(str));
	}

	/* 按字典序遍历单词树 */
	template <typename Functor>
	void traverse(Functor &execute = Functor())
	{
		visit_node(root, execute);
	}

private:
	/* 访问某结点及其子结点 */
	template <typename Functor>
	void visit_node(node_type cur, Functor &execute)
	{
		execute(cur);
		for (int i = 0; i < Size; ++i)
		{
			if (cur.child[i] == 0) continue;
			visit_node(*cur.child[i], execute);
		}
	}
	/* 清除某个节点的所有子节点 */
	void clear_node(node_type cur)
	{
		for (int i = 0; i < Size; ++i)
		{
			if (cur.child[i] == 0) continue;
			clear_node(*cur.child[i]);
			delete cur.child[i];
			cur.child[i] = 0;
			if (--cur.node == 0) break;
		}
	}

	/* 边搜索边删除冗余节点，返回值用于向其父节点声明是否该删除该节点 */
	template <typename Iterator>
	bool erase_node(Iterator begin, Iterator end, node_type &cur, bool &result)
	{
		if (begin == end) //当到达字符串结尾：递归的终止条件
		{
			result = cur.terminable; //如果当前节点可以作为终止字符，那么结果为真
			cur.terminable = false;  //设置该节点为不可作为终止字符，即删除该字符串
			return cur.node == 0;    //若该节点为树叶，那么通知其父节点删除它
		}
		//当无法匹配当前字符时，将结果设为假并返回假，即通知其父节点不要删除它
		if (cur.child[index[*begin]] == 0) return result = false;
		//判断是否应该删除该子节点
		else if (erase_node((++begin)--, end, *(cur.child[index[*begin]]), result))
		{
			delete cur.child[index[*begin]]; //删除该子节点
			cur.child[index[*begin]] = 0; //子节点数减一
			//若当前节点为树叶，那么通知其父节点删除它
			if (--cur.node == 0 && cur.terminable == false) return true;
		}
		return false; //其他情况都返回假
	}

	/* 根节点 */
	node_type root;

	/* 将字符转换为索引的转换表或函数对象 */
	Index index;
};

//index function object
class IndexClass
{
public:
	int operator[](const char key)
	{
		if (key >= '0'&&key <= '9')
		{
			return key - '0';
		}
		else if (key>='a'&&key<='f')
		{
			return key - 'a' + 10;
		}
		else
		{
			return key - 'A' + 10;
		}
	}
};



/*
int main()
{
    trie<26,IndexClass> t;
    t.insert("tree");
    t.insert("tea");
    t.insert("A");
    t.insert("ABC");

    if(t.find("tree"))
        cout<<"find tree"<<endl;
    else
        cout<<"not find tree"<<endl;

    if(t.find("tre"))
        cout<<"find tre"<<endl;
    else
        cout<<"not find tre"<<endl;
    
    if(t.erase("tree"))
        cout<<"delete tree"<<endl;
    else
        cout<<"not find tree"<<endl;

    if(t.find("tree"))
        cout<<"find tree"<<endl;
    else
        cout<<"not find tree"<<endl;

    return 0;
}
*/
