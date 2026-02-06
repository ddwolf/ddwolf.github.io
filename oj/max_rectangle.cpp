/**
 * ai 在某区域 r 是最小的，此区域的宽度*ai。
 * 找到最大的 r*ai
 * for i in 0 .. N-1:
 *     for j in 1 .. N-1:
 *         if a[j] < a[i]:
 *             right = j - 1
 *             break
 *     for j in i-1 .. 0:
 *         if a[j] < a[i]:
 *             left = j + 1
 *             break
 *     ret = max(ret, a[i] * (right - left + 1))
 * 这个是暴力解法，但也能AC
 * 官方解法是通过单调递增栈。从左向右遍历，遇到更小的，就弹出元素，直到遇到比它小的，
 * 左边第一个比它小的，就是可能的左边界。右边也同样遍历 一次。
 */
class Solution {
public:
    int largestRectangleArea(vector<int>& heights) {
        return largestRectangleAreaStack(heights);
    }
    int largestRectangleAreaStack(vector<int>& heights) {
        const int N = heights.size();
        int ret = 0;
        vector<int> leftvec(heights.size(), 0);
        vector<int> rightvec(heights.size(), 0);
        vector<std::pair<int, int>> leftstack, rightstack;
        for (int i = 0; i < N; ++i) {
            while (leftstack.size() > 0 and (leftstack[leftstack.size() - 1]).first >= heights[i]) {
                leftstack.pop_back();
            }
            while (rightstack.size() > 0 and (rightstack[rightstack.size() - 1]).first >= heights[N - i - 1]) {
                rightstack.pop_back();
            }
            if (leftstack.size() == 0) {
                leftvec[i] = -1;
            } else {
                leftvec[i] = (leftstack[leftstack.size() - 1]).second;
            }
            if (rightstack.size() == 0) {
                rightvec[N - i - 1] = N;
            } else {
                rightvec[N - i - 1] = rightstack[rightstack.size() - 1].second;
            }
            leftstack.emplace_back(heights[i], i);
            rightstack.emplace_back(heights[N - i - 1], N - i - 1);
        }
        for (int i = 0; i < N; ++i) {
            ret = max(ret, (rightvec[i] - leftvec[i] - 1) * heights[i]);
            // printf("(%d,%d,%d)\n", leftvec[i], rightvec[i], ret);
        }
        return ret;
    }

    int largestRectangleAreaBruteForce(vector<int>& heights) {
        const int N = heights.size();
        vector<bool> calculated(N, false);
        int ret = 0;
        int right = 0;
        int left = 0;
        for (int i = 0; i < N; ++i) {
            if (calculated[i]) {
                continue;
            }
            left = i;
            right = i;
            for (int j = i + 1; j < N; ++j) {
                if (heights[j] == heights[i]) {
                    calculated[j] = true;
                }
                if (heights[j] < heights[i]) {
                    break;
                }
                //printf("... %d >= %d\n", heights[j], heights[i]);
                ++right;
            }
            for (int j = i - 1; j >= 0; --j) {
                if (heights[j] == heights[i]) {
                    calculated[j] = true;
                }
                if (heights[j] < heights[i]) {
                    break;
                }
                --left;
            }
            ret = max(ret, heights[i] * (right - left + 1));
           // printf("%d ::(%d,%d) %d\n", i, left, right, ret);
        }
        return ret;
    }
};
