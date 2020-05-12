#ifndef __SMAP_H__
#define __SMAP_H__

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "smap.h"

#define SMAP_INITIAL_CAPACITY 100

typedef struct entry
{
  char *key;
  void *value;
  bool occupied;
  bool deleted;
} entry;



typedef struct smap
{
	int capacity;
	int count;
	int (*hash)(const char *);
	entry *entries;
} smap;



/**
 * Returns a hash value for the given string.
 *
 * @param s a string, non-NULL
 * @return an int
 */
int smap_default_hash(const char *s)
{
	unsigned long h;
	unsigned const char *us;

	us = (unsigned const char *) s;
	h = 0;

	while (*us != '\0')
	{
		h = h * 37 + *us;
		us++; 
	}
	return h;
}

/**
 * Creates an empty map that uses the given hash function.
 *
 * @param h a pointer to a function that takes a string and returns
 * its hash code, non-NULL
 * @return a pointer to the new map or NULL if it could not be created;
 * it is the caller's responsibility to destroy the map
 */
smap *smap_create(int (*h)(const char *s))
{
	if (h != NULL)
	{
		smap *result = malloc(sizeof(smap));

		if (result != NULL)
		{
		  result->count = 0;
		  result->hash = h;
		  result->entries = malloc(sizeof(entry) * SMAP_INITIAL_CAPACITY);
		  result->capacity = (result->entries != NULL ? SMAP_INITIAL_CAPACITY : 0);

		  for (int i = 0; i < result->capacity; i++)
		  {
		  	result->entries[i].occupied = false;
		  	result->entries[i].deleted = false;
		  }
		}
		else
		{
			free(result);
			exit(0);
		}
	  	return result;
	}
	return NULL;
}

/**
 * Returns the number of (key, value) pairs in the given map.
 *
 * @param m a pointer to a map, non-NULL
 * @return the size of m
 */
int smap_size(const smap *m)
{
	return (m == NULL ? 0 : m->count);
}

// returns index of key in entries
int smap_table_get_key_index(const entry *entries, const char *key, int (*hash)(const char *), int capacity)
{
  	int i = (capacity + hash(key) % capacity) % capacity;

  	// accounting for open addressing
  	while (entries[i].occupied && (strcmp(entries[i].key, key) != 0))
  	{
   		i = (i + 1) % capacity;
 		}

  	return i;
}

/**
 * Adds a copy of the given key with value to this map.
 * If the key is already present then the old value is replaced.
 * The caller retains ownership of the value.  If key is new
 * and space could not be allocated then there is no effect on the map
 * and the return value is false.
 *
 * @param entries a pointer to a map's entries, non-NULL
 * @param key a string, non-NULL
 * @param value a pointer
 * @return true if the put was successful, false otherwise
 */
void smap_table_add(entry *entries, const char *key, void *value, int (*hash)(const char *), int capacity)
{
	int index = smap_table_get_key_index(entries, key, hash, capacity);
	char *new_key = malloc(strlen(key) + 1);

	strcpy(new_key, key);

	entries[index].key = new_key;
	entries[index].value = value;
	entries[index].occupied = true;
	entries[index].deleted = false;
}

void smap_embiggen(smap *m)
{
	entry *bigger = malloc(sizeof(entry) * m->capacity*2);

	if (bigger != NULL)
	{
		for (int i = 0; i < m->capacity*2; i++)
		{
			bigger[i].occupied = false;
			bigger[i].deleted = false;
		}
		for (int i = 0; i < m->capacity; i++)
		{
			if (m->entries[i].occupied && !m->entries[i].deleted)
			{
				smap_table_add(bigger, m->entries[i].key, m->entries[i].value, m->hash, m->capacity*2);
				free(m->entries[i].key);		
			}	
		}
	}
	else
	{
		free(bigger);
		exit(0);
	}
	entry *temp = m->entries;
	m->entries = bigger;

	m->capacity *= 2;
	free(temp);
}

void smap_shrink(smap *m)
{
	entry *smaller = malloc(sizeof(entry) * m->capacity/2);

	if (smaller != NULL)
	{
		for (int i = 0; i < m->capacity/2; i++)
		{
			smaller[i].occupied = false;
  		smaller[i].deleted = false;
		}
		for (int i = 0; i < m->capacity; i++)
		{
			if (m->entries[i].occupied && !m->entries[i].deleted)
			{
				smap_table_add(smaller, m->entries[i].key, m->entries[i].value, m->hash, m->capacity/2);
				free(m->entries[i].key);		
			}	
		}
	}
	else
	{
		free(smaller);
		exit(0);
	}
	entry *temp = m->entries;
	m->entries = smaller;
	
	m->capacity /= 2;
	free(temp);
}

// add function, but accounting for possible need to embiggen
bool smap_put(smap *m, const char *key, void *value)
{
	int i = smap_table_get_key_index(m->entries, key, m->hash, m->capacity);

	// updating value of already-put key
	if (m->entries[i].occupied)
	{
		m->entries[i].value = value;
		return true;
	}
	else // new key
	{
		if (m->count > m->capacity / 2) // if we need to make the hash table bigger
		{
			smap_embiggen(m);
		}

		if (m->count < m->capacity) // there is room
		{
			smap_table_add(m->entries, key, value, m->hash, m->capacity);
			m->count++;
			return true;
		}
		else // no room
		{
			return false;
		}
	}	
}

/**
 * Determines if the given key is present in this map.
 *
 * @param m a pointer to a map, non-NULL
 * @param key a string, non-NULL
 * @return true if key is present in this map, false otherwise
 */
bool smap_contains_key(const smap *m, const char *key)
{
	int i = smap_table_get_key_index(m->entries, key, m->hash, m->capacity);
	return (m->entries[i].occupied && !m->entries[i].deleted && (strcmp(m->entries[i].key, key) == 0));
}

/**
 * Returns the value associated with the given key in this map.
 * If the key is not present in this map then the returned value is
 * NULL.  The value returned is the original value passed to smap_put,
 * and it remains the responsibility of whatever called smap_put to
 * release the value (no ownership transfer results from smap_get).
 *
 * @param m a pointer to a map, non-NULL
 * @param key a string, non-NULL
 * @return the assocated value, or NULL if they key is not present
 */
void *smap_get(smap *m, const char *key)
{
	int i = smap_table_get_key_index(m->entries, key, m->hash, m->capacity);
	if (m->entries[i].occupied)
	{
		return m->entries[i].value;
	}
	return NULL;
}

/**
 * Removes the given key and its associated value from the given map if
 * the key is present.  The return value is NULL and there is no effect
 * on the map if the key is not present.
 *
 * @param m a pointer to a map, non-NULL
 * @param key a key, non-NULL
 * @return the value associated with the key
 */
void *smap_remove(smap *m, const char *key)
{
	int i = smap_table_get_key_index(m->entries, key, m->hash, m->capacity);
	if (!m->entries[i].occupied)
	{
		return NULL;
	}
	
	void *value = m->entries[i].value;
	free(m->entries[i].key);
	m->entries[i].occupied = false;
	m->entries[i].deleted = true;
	m->count--;

	if (m->count < m->capacity/8)
	{
		smap_shrink(m);
	}

	return value;

}

/**
 * Calls the given function for each (key, value) pair in this map, passing
 * the extra argument as well.  This function does not add or remove from
 * the map.
 *
 * @param m a pointer to a map, non-NULL
 * @param f a pointer to a function that takes a key, a value, and an
 * extra argument, and does not add to or remove from the map, no matter
 * what the extra argument is; non-NULL
 * @param arg a pointer
 */
void smap_for_each(smap *m, void (*f)(const char *, void *, void *), void *arg)
{
  	if (m == NULL || f == NULL)
    {
      	return;
    }

  	for (int i = 0; i < m->capacity; i++)
    {
    	if (m->entries[i].occupied && !m->entries[i].deleted)
    	{
      		f(m->entries[i].key, m->entries[i].value, arg);
    	}
    }
}

/**
 * Returns a dynamically-alloc'd array with pointers to the keys in the
 * given map.  It is the caller's responsibility to free the array,
 * but the map retains ownership of the keys.  If there is a memory
 * allocation error then the returned value is NULL.  If the map is empty
 * then the returned value is NULL.
 *
 * @param m a pointer to a map, non NULL
 * @return a pointer to a dynamically alloc'd array of pointer to the keys
 */
const char **smap_keys(smap *m)
{
	if (m->count < 1)
	{
		return NULL;
	}

	const char **keys = malloc(m->count * sizeof(char*));
	if (keys == NULL)
	{
		free(keys);
		exit(0);
	}
	int count = 0;

	for (int i = 0; i < m->capacity; i++)
	{
		if (m->entries[i].occupied && m->entries[i].key != NULL)
		{
			keys[count] = m->entries[i].key;
			count++;
		}
	}

	return keys;
}

/**
 * Destroys the given map.
 *
 * @param m a pointer to a map, non-NULL
 */
void smap_destroy(smap *m)
{
	for (int i = 0; i < m->capacity; i++)
	{
		if (m->entries[i].occupied && m->entries[i].key != NULL)
		{			
			free(m->entries[i].key);
		}
	}
	free(m->entries);
	free(m);
}

#endif
