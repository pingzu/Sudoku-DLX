#include <iostream>
#include <chrono>
#include <iomanip>
using namespace std;
using namespace std::chrono;

class SudokuSolver {
public:
	const enum State { DEFAULT = 1, SUCCEED = 0, ILLEGAL_VALUE = -1, CONFLICT = -2, NO_ANSWER = -3, OTHER = -4 };

	int solve(int(&sudoku)[9][9])
	{
		copy_sudoku(answer, sudoku);			//复制数据到内部数组
		int state;
		if ((state = check_data()) != DEFAULT)
			return state;
		if ((state = buildLinks()) != DEFAULT)
			return state;
		if ((state = search_answer()) != SUCCEED)
			return state;
		copy_sudoku(sudoku, answer);
		return state;
	}

protected:
	int answer[9][9];

	struct Node {						//十字链结点
		union {
			Node  *col;					//列头指针
			int	  cnt;					//列结点个数
		};
		Node  *up;
		Node  *down;
		Node  *left;
		Node  *right;
	};
	struct {
		Node col_heads[324];			//列指针
		Node head;		  				//十字链表头指针
		Node row_heads[729][4];			//行指针
	}Links;


	Node* select_col()					//选取结点个数最少的一列(递归次数少)
	{
		Node *p = Links.head.right;
		for (Node *q = p; q != &Links.head; q = q->right) {
			if (q->cnt == 0)			//结点个数为0说明无解
				return nullptr;
			else if (q->cnt == 1) {
				p = q;
				break;
			}
			else if (q->cnt < p->cnt)
				p = q;
		}
		return p;
	}
	void fill_num(Node *target_row)		//根据答案行把对应的数字填入数组中
	{
		while ((target_row->col - Links.col_heads) > 80)				//前81列代表在数独中的位置
			--target_row;
		int index = target_row->col - Links.col_heads;
		int num = (target_row->right->col - Links.col_heads - 80) % 9;
		if (!num)	//特殊情况
			num = 9;
		answer[index / 9][index % 9] = num;		//第index个格子的答案是num
	}
	int search_answer()					//递归求解 
	{
		if (Links.head.right == &Links.head)	//表中无剩余结点,求解成功
			return SUCCEED;
		Node *p = select_col();
		if (p == nullptr)
			return NO_ANSWER;
		Node *h = p;

		removeNodes(h);
		p = p->up;
		while (p != h) {
			for (Node *t = p->left; t != p; t = t->left)	//将当前行作为答案，从十字链表中移除
				removeNodes(t->col);
			if (search_answer() == SUCCEED) {				//找到答案,将当前行代表的数字填入数组中
				fill_num(p);
				return SUCCEED;
			}
			for (Node *t = p->right; t != p; t = t->right)	//当前行不是答案，从十字链表中恢复
				resumeNodes(t->col);
			p = p->up;										//尝试下一行
		}
		resumeNodes(h);
		return NO_ANSWER;
	}

	void copy_sudoku(int(&lhs)[9][9], int(&rhs)[9][9])
	{
		for (int i = 0; i != 9; ++i)
			for (int j = 0; j != 9; ++j)
				lhs[i][j] = rhs[i][j];
	}

	void removeNodes(Node *col_head)const
	{
		col_head->left->right = col_head->right;
		col_head->right->left = col_head->left;
		for (Node *p = col_head->down; p != col_head; p = p->down) {
			for (Node *q = p->right; q != p; q = q->right) {
				q->up->down = q->down;
				q->down->up = q->up;
				--q->col->cnt;
			}
		}
	}
	void resumeNodes(Node *col_head)const
	{
		col_head->left->right = col_head;
		col_head->right->left = col_head;
		for (Node *p = col_head->down; p != col_head; p = p->down) {
			for (Node *q = p->right; q != p; q = q->right) {
				q->up->down = q;
				q->down->up = q;
				++q->col->cnt;
			}
		}
	}

	int check_data()const			//检查数据合法性
	{
		for (int i = 0; i != 9; ++i)
			for (int j = 0; j != 9; ++j) {
				if (answer[i][j] < 0 || answer[i][j] > 9)
					return ILLEGAL_VALUE;
			}
		return DEFAULT;
	}
	int buildLinks()				//构建舞蹈链
	{
		int state;
		buildLinks_addColHead();	//添加列头结点
		if ((state = buildLinks_markSolvedCell()) != DEFAULT)	//标记已知的数字
			return state;
		buildLinks_addCell();		//将剩余数字加入表中
		return DEFAULT;
	}
	void buidlLinks_calcIndex(int(&indexs)[4], int *cell)const	//计算对应的列索引
	{
		indexs[0] = cell - *answer;
		indexs[1] = indexs[0] / 9 * 9 + *cell + 80;
		indexs[2] = indexs[0] % 9 * 9 + *cell + 161;
		indexs[3] = (indexs[0] / 9 / 3 * 3 + indexs[0] % 9 / 3) * 9 + *cell + 242;
	}
	void buildLinks_addColHead()
	{
		Links.head.right = Links.col_heads;
		for (Node *ptr = Links.col_heads; ptr != Links.col_heads + 324; ++ptr) {
			if (ptr != Links.col_heads)
				ptr->left = ptr - 1;
			else
				ptr->left = &Links.head;
			if (ptr != Links.col_heads + 323)
				ptr->right = ptr + 1;
			else
				ptr->right = &Links.head;
			ptr->up = ptr;
			ptr->down = ptr;
			ptr->cnt = 0;
		}
	}
	int buildLinks_markSolvedCell()
	{
		for (int *cell = *answer, indexs[4]; cell != *answer + 81; ++cell) {
			if (*cell == 0)
				continue;
			buidlLinks_calcIndex(indexs, cell);
			for (int i = 0; i < 4; ++i) {
				Node *ptr = Links.col_heads + indexs[i];
				if (ptr->cnt == -1)
					return CONFLICT;
				ptr->cnt = -1;					//标记已完成的列
				ptr->right->left = ptr->left;
				ptr->left->right = ptr->right;
			}
		}
		return DEFAULT;
	}
	void buildLinks_addCell()
	{
		Node(*row_ptr)[4] = Links.row_heads;
		for (int *ptr = *answer, col_index[4]; ptr != *answer + 81; ++ptr) {
			if (*ptr != 0)
				continue;
			buidlLinks_calcIndex(col_index, ptr);
			for (int num = 1; num < 10; ++num) {
				++col_index[1], ++col_index[2], ++col_index[3];
				if (Links.col_heads[col_index[1]].cnt == -1 || Links.col_heads[col_index[2]].cnt == -1 || Links.col_heads[col_index[3]].cnt == -1)
					continue;
				Node *rows = *row_ptr;
				for (int i = 0; i != 4; ++i) {
					if (i == 0)
						rows[i].left = &rows[3];
					else
						rows[i].left = &rows[i - 1];
					if (i == 3)
						rows[i].right = &rows[0];
					else
						rows[i].right = &rows[i + 1];
					Node *q = Links.col_heads + col_index[i];
					rows[i].up = q->up;
					rows[i].down = q;
					rows[i].col = q;
					q->up->down = &rows[i];
					q->up = &rows[i];
					++q->cnt;
				}
				++row_ptr;
			}
		}
	}
};


void input_data(int(&buffer)[9][9])		//确定数据输入格式
{
	int type = 0;
	/*
	type = 0:
		0 8 0 0 0 0 0 9 4
		0 0 0 0 5 7 0 0 0
		0 1 0 0 0 0 0 0 0
		7 0 0 4 0 6 0 0 0
		2 0 3 0 0 0 0 0 0
		5 0 0 0 0 0 0 0 0
		0 0 0 0 3 0 5 7 0
		0 0 0 0 0 0 2 0 0
		0 0 0 6 0 0 0 0 0
		or
		0 8 0 0 0 0 0 9 4 0 0 0 0 5 7 0 0 0 0 1 0 0 0 0 0 0 0 7 0 0 4 0 6 0 0 0 2 0 3 0 0 0 0 0 0 5 0 0 0 0 0 0 0 0 0 0 0 0 3 0 5 7 0 0 0 0 0 0 0 2 0 0 0 0 0 6 0 0 0 0 0

	type = 1:
		080000094000057000010000000700406000203000000500000000000030570000000200000600000
		.8.....94....57....1.......7..4.6...2.3......5............3.57.......2.....6.....
		*8*****94****57****1*******7**4*6***2*3******5************3*57*******2*****6*****
		...
	*/
	if (type != 1) {
		for (int row = 0; row < 9; row++)
			for (int col = 0; col < 9; col++)
				cin >> buffer[row][col];
	}
	else if (type == 1) {
		for (int row = 0; row < 9; row++)
			for (int col = 0; col < 9; col++) {
				char ch = cin.get();
				buffer[row][col] = (ch >= '1'&&ch <= '9') ? ch - '0' : 0;
			}
	}
}

int main()
{
	int buffer[9][9], sudoku[9][9];
	int row, col;
	int ret;
	input_data(buffer);

	/* 输出初始值 */
	cout << "Begin" << endl;
	for (row = 0; row < 9; row++) {
		for (col = 0; col < 9; col++)
			cout << buffer[row][col] << ' ';
		cout << endl;
	}
	cout << "End" << endl;

	const int frequency = 500;	//重复试验次数
	SudokuSolver s;
	auto beg = chrono::high_resolution_clock::now();
	for (int i = 0; i != frequency; ++i) {		//重新赋值重复试验frequency次
		for (int j = 0; j != 9; ++j)
			for (int k = 0; k != 9; ++k)
				sudoku[j][k] = buffer[j][k];
		ret = s.solve(sudoku);
	}
	auto end = chrono::high_resolution_clock::now();

	switch (ret) {
		case SudokuSolver::SUCCEED:
			cout << "Answer Begin" << endl;
			for (row = 0; row < 9; row++) {
				for (col = 0; col < 9; col++)
					cout << sudoku[row][col] << ' ';
				cout << endl;
			}
			cout << "Answer End" << endl;
			break;

		case SudokuSolver::ILLEGAL_VALUE:
			cout << "数据有非法值" << endl;
			break;


		case SudokuSolver::NO_ANSWER:
			cout << "无解" << endl;
			break;

		default:
			cout << "有其它错误" << endl;
			break;
	} //end of switch
	cout << setw(15) << setiosflags(ios::left) << "Test Frequency" << ':' << frequency << endl;
	cout << setw(15) << setiosflags(ios::left | ios::fixed) << setprecision(9) << "Total Time" << ':'
		<< (double)(end - beg).count() / chrono::high_resolution_clock::period::den
		<< " s" << endl;
	cout << setw(15) << setiosflags(ios::left | ios::fixed) << setprecision(9) << "Average Time" << ':'
		<< (double)(end - beg).count() / frequency / chrono::high_resolution_clock::period::den
		<< " s" << endl;

	return 0;
}
