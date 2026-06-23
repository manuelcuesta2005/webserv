# ==============================================================================
#                               CONFIGURACIÓN
# ==============================================================================

NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC_DIR     = src
OBJ_DIR     = obj

# ==============================================================================
#                        DETECCIÓN DINÁMICA DE ARCHIVOS
# ==============================================================================

# Busca TODOS los archivos .cpp dentro de la carpeta 'src' y sus subcarpetas
SRCS        = $(shell find $(SRC_DIR) -name "*.cpp")

# Transforma la lista de 'src/camino/archivo.cpp' a 'obj/camino/archivo.o'
OBJS        = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# ==============================================================================
#                                   REGLAS
# ==============================================================================

all: $(NAME)

# Enlace del ejecutable final unificado
$(NAME): $(OBJS)
	@echo "🔗 Enlazando el servidor central $(NAME)..."
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "🚀 ¡Webserv listo para correr! Ejecuta: ./$(NAME)"

# Regla genérica que mantiene la estructura de subcarpetas en 'obj/'
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "🧱 Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpieza de objetos
clean:
	@echo "🧹 Limpiando archivos objeto..."
	rm -rf $(OBJ_DIR)

# Limpieza total (objetos + ejecutable)
fclean: clean
	@echo "🗑️ Eliminando ejecutable..."
	rm -f $(NAME)

# Re-compilación desde cero
re: fclean all

.PHONY: all clean fclean re