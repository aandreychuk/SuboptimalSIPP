#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "structs.h"
#include "gl_const.h"
#include <vector>
#include <unordered_map>
#include "map.h"

typedef multi_index_container<
        Node,
        indexed_by<
                    //ordered_non_unique<tag<cost>, BOOST_MULTI_INDEX_MEMBER(Open_Elem, double, cost)>,
                    ordered_non_unique<BOOST_MULTI_INDEX_MEMBER(Node, double, g)>,
                    hashed_unique<BOOST_MULTI_INDEX_MEMBER(Node, int, open_id)>
        >
> Container;


class Heuristic
{
    std::vector<std::vector<std::vector<double>>> h_values;
    int connectedness;
    int focaltype;
    double size;
    Container open;
    std::vector<std::vector<Node>> optimal_paths;
    void add_open(Node newNode);
    double dist(const Node& a, const Node& b){ return std::sqrt(pow(a.i - b.i, 2) + pow(a.j - b.j, 2)); }
    Node find_min()
    {
        Node min = *open.begin();
        open.erase(open.begin());
        return min;
    }
public:
    Heuristic(int connectdeness_ = CN_DEFAULT_CONNECTEDNESS, int focaltype_ = CN_FOCAL_HTG, double size_ = CN_DEFAULT_SIZE)
        :connectedness(connectdeness_), focaltype(focaltype_), size(size_){}
    void init(int width, int height, int agents)
	{
        h_values.clear();
        h_values.resize(height);
        for(int i = 0; i < height; i++)
        {
            h_values[i].resize(width);
            for(int j = 0; j < width; j++)
                h_values[i][j].resize(agents,-1);
		}
        optimal_paths.resize(agents);
    }
    void findStaticPath(const Map &map, Agent agent)
    {
        Node curNode(agent.start_i, agent.start_j, 0, 0), newNode;
        curNode.open_id = curNode.i * map.width + curNode.j;
        curNode.Parent = nullptr;
        open.clear();
        open.insert(curNode);
        std::unordered_map<int, Node> closed;
        while(!open.empty())
        {
            curNode = find_min();
            closed.insert(std::make_pair(curNode.open_id, curNode));
            if(curNode.i == agent.goal_i && curNode.j == agent.goal_j)
                break;
            std::vector<Node> valid_moves = map.getValidMoves(curNode.i, curNode.j, connectedness, size);
            for(auto move: valid_moves)
            {
                newNode.i = curNode.i + move.i;
                newNode.j = curNode.j + move.j;
                newNode.open_id = newNode.i * map.width + newNode.j;
                newNode.g = curNode.g + dist(curNode, newNode);
                newNode.Parent = &closed.find(curNode.open_id)->second;
                if(closed.find(newNode.open_id) == closed.end())
                {
                    auto it = open.get<1>().find(newNode.open_id);
                    if(it != open.get<1>().end())
                    {
                        if(it->g > newNode.g)
                            open.get<1>().erase(it);
                        else
                            continue;
                    }
                    open.insert(newNode);
                }
            }
        }
        if(!open.empty())
        {
            std::vector<Node> path;
            while(curNode.Parent != nullptr)
            {
                path.insert(path.begin(), curNode);
                curNode = *curNode.Parent;
            }
            path.insert(path.begin(), curNode);
            optimal_paths[agent.id_num] = path;
        }
    }
    void count(const Map &map, Agent agent)
	{
        if(focaltype == CN_FOCAL_HTG)
        {
            Node curNode(agent.goal_i, agent.goal_j, 0, 0), newNode;
            curNode.open_id = curNode.i * map.width + curNode.j;
            open.clear();
            open.insert(curNode);
            while(!open.empty())
            {
                curNode = find_min();
                h_values[curNode.i][curNode.j][agent.id_num] = curNode.g;
                std::vector<Node> valid_moves = map.getValidMoves(curNode.i, curNode.j, connectedness, size);
                for(auto move: valid_moves)
                {
                    newNode.i = curNode.i + move.i;
                    newNode.j = curNode.j + move.j;
                    newNode.open_id = newNode.i * map.width + newNode.j;
                    newNode.g = curNode.g + 1;//dist(curNode, newNode);
                    if(h_values[newNode.i][newNode.j][agent.id_num] < 0)
                    {
                        auto it = open.get<1>().find(newNode.open_id);
                        if(it != open.get<1>().end())
                        {
                            if(it->g > newNode.g)
                                open.get<1>().erase(it);
                            else
                                continue;
                        }
                        open.insert(newNode);
                    }
                }
            }
        }
        else
            findStaticPath(map, agent);
    }

    double get_value(int i, int j, double g, int agent_id)
    {
        if(focaltype == CN_FOCAL_DISTANCE)
        {
            double dist_from_opt = CN_INFINITY;
            for (auto & t : optimal_paths[agent_id]) {
                if (t.i == i && t.j == j){
                    return 0.0;
                }
                double dist = std::sqrt(pow(t.i - i, 2) + pow(t.j - j, 2));
                if (dist < dist_from_opt) {
                    dist_from_opt = dist;
                }
            }
            return dist_from_opt;
        }
        else if(focaltype == CN_FOCAL_TIME)
        {
            Node nearest_node = optimal_paths[agent_id].front();
            for (auto & t : optimal_paths[agent_id])
                if(fabs(t.g - g) < fabs(nearest_node.g - g))
                    nearest_node = t;
            return std::sqrt(pow(nearest_node.i - i, 2) + pow(nearest_node.j - j, 2));
        }
        else// if(focaltype == CN_FOCAL_HTG)
            return h_values[i][j][agent_id];
	}

};

#endif // HEURISTIC_H
