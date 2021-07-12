/* Name, library.c, CS 24000, Spring 2020
 * Last updated March 27, 2020
 */

/* Add any includes here */

#include "../include/library.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <regex.h>
#include <ftw.h>
#include "../include/parser.h"

tree_node_t *g_song_library = NULL;

/*
 * Define find_parent_pointer here
 */

tree_node_t **find_parent_pointer(tree_node_t **root_ptr,
                                  const char *song_name) {
    tree_node_t *root = *root_ptr;
    if (!root_ptr) {
        return NULL;
    }
    int status = strcmp(root->song_name, song_name);
    if (status == 0) {
        return root_ptr;
    } else if (status < 0) {
        if (root->right_child) {
            if (strcmp(root->right_child->song_name, song_name) == 0) {
                return &root->right_child;
            } else {
                return find_parent_pointer(&root->right_child, song_name);
            }
        } else {
            return NULL;
        }
    } else {
        if (root->left_child) {
            if (strcmp(root->left_child->song_name, song_name) == 0) {
                return &root->left_child;
            } else {
                return find_parent_pointer(&root->left_child, song_name);
            }
        } else {
            return NULL;
        }
    }
} /* find_parent_pointer() */

/*
 * Define tree_insert here
 */

int tree_insert(tree_node_t **root_ptr, tree_node_t *new) {
    if (*root_ptr == NULL) {
        *root_ptr = new;
        return INSERT_SUCCESS;
    }
    tree_node_t *root = *root_ptr;
    int status = strcmp(root->song_name, new->song_name);
    if (status == 0) {
        return DUPLICATE_SONG;
    } else if (status < 0) {
        if (root->right_child) {
            return tree_insert(&root->right_child, new);
        } else {
            root->right_child = new;
            return INSERT_SUCCESS;
        }
    } else {
        if (root->left_child) {
            return tree_insert(&root->left_child, new);
        } else {
            root->left_child = new;
            return INSERT_SUCCESS;
        }
    }
} /* tree_insert() */

/*
 * Define remove_song_from_tree here
 */

int remove_song_from_tree(tree_node_t **root_ptr, const char *song_name) {
    assert(song_name != NULL);
    tree_node_t **parent = find_parent_pointer(root_ptr, song_name);
    if (parent == NULL) {
        return SONG_NOT_FOUND;
    }
    tree_node_t *left = (*parent)->left_child;
    tree_node_t *right = (*parent)->right_child;
    free_node(*parent);
    *parent = NULL;
    if (left) {
        tree_insert(root_ptr, left);
    }
    if (right) {
        tree_insert(root_ptr, right);
    }
    return DELETE_SUCCESS;
} /* remove_song_from_tree() */

/*
 * Define free_node here
 */

void free_node(tree_node_t *node) {
    free_song(node->song);
    node->left_child = NULL;
    node->right_child = NULL;
    free(node);
} /* free_node() */

/*
 * Define print_node here
 */

void print_node(tree_node_t *node, FILE *file) {
    assert(file != NULL);
    assert(node != NULL);
    fprintf(file, "%s\n", node->song_name);
} /* print_node() */

/*
 * Define traverse_pre_order here
 */

void traverse_pre_order(tree_node_t *root, void *arb,
                        traversal_func_t function) {
    assert(function != NULL);
    function(root, arb);
    if (root->left_child) {
        traverse_pre_order(root->left_child, arb, function);
    }
    if (root->right_child) {
        traverse_pre_order(root->right_child, arb, function);
    }
} /* traverse_pre_order() */

/*
 * Define traverse_in_order here
 */

void traverse_in_order(tree_node_t *root, void *arb,
                       traversal_func_t function) {
    assert(function != NULL);
    if (root->left_child) {
        traverse_pre_order(root->left_child, arb, function);
    }
    function(root, arb);
    if (root->right_child) {
        traverse_pre_order(root->right_child, arb, function);
    }
} /* traverse_in_order() */

/*
 * Define traverse_post_order here
 */

void traverse_post_order(tree_node_t *root, void *arb,
                         traversal_func_t function) {
    assert(function != NULL);
    if (root->left_child) {
        traverse_pre_order(root->left_child, arb, function);
    }
    if (root->right_child) {
        traverse_pre_order(root->right_child, arb, function);
    }
    function(root, arb);
} /* traverse_post_order() */

/*
 * Define free_library here
 */

void free_library(tree_node_t *root) {
    assert(root != NULL);
    if (root->left_child) {
        free_library(root->left_child);
        root->left_child = NULL;
    }
    if (root->right_child) {
        free_library(root->right_child);
        root->right_child = NULL;
    }
    free_node(root);
} /* free_library() */

/*
 * Define write_song_list here
 */

void write_song_list(FILE *file, tree_node_t *head) {
    traverse_in_order(head, file, (traversal_func_t) print_node);
} /* write_song_list() */

/*
 * parse file
 */

int parse(const char *file_path, const struct stat *ptr, int flags) {
    if ((file_path[strlen(file_path) - 4] == '.') &&
        (file_path[strlen(file_path) - 3] == 'm') &&
        (file_path[strlen(file_path) - 2] == 'i') &&
        (file_path[strlen(file_path) - 1] == 'd')) {
        tree_node_t *new = malloc(sizeof(tree_node_t));
        new->song = parse_file(file_path);
        new->left_child = NULL;
        new->right_child = NULL;
        new->song_name = new->song->path;
        char *ptr = NULL;
        while ((ptr = strchr(new->song_name, '/')) != NULL) {
            new->song_name = ptr + 1;
        }
        printf("%s\n", new->song_name);
        tree_insert(&g_song_library, new);
    }
    return 0;
} /* parse() */

/*
 * Define make_library here
 */

void make_library(const char *dir) {
    ftw(dir, parse, 100);
} /* make_library() */
