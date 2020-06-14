/**
 * lr(1)
 * @author Tobias Weber
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __AST_H__
#define __AST_H__

#include <memory>
#include <functional>


/**
 * syntax tree base
 */
class ASTBase
{
};

using t_astbaseptr = std::shared_ptr<ASTBase>;


// semantic rule: returns an ast pointer and gets a vector of ast pointers
using t_semanticrule = std::function<t_astbaseptr(const std::vector<t_astbaseptr>&)>;


#endif
