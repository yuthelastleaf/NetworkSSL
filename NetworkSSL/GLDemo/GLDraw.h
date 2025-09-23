#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

#include "shader.h"

class GLDraw
{
public:
    GLDraw()
        : vertexShader(0)
        , fragmentShader(0)
        , shaderProgram(0)
        , VAO(0)
        , VBO(0)
        , EBO(0)
        , outshader(nullptr)
    {

    }
    ~GLDraw() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
        }
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
        }
        if (outshader) {
            delete outshader;
        }
    }

	void InitTrangle() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);

	}

    void InitRectangle() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
            0.5f, 0.5f, 0.0f,   // ���Ͻ�
            0.5f, -0.5f, 0.0f,  // ���½�
            -0.5f, -0.5f, 0.0f, // ���½�
            -0.5f, 0.5f, 0.0f   // ���Ͻ�
        };

        unsigned int indices[] = {
            // ע��������0��ʼ! 
            // ����������(0,1,2,3)���Ƕ�������vertices���±꣬
            // �����������±��������ϳɾ���

            0, 1, 3, // ��һ��������
            1, 2, 3  // �ڶ���������
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }

    void InitNearTrangle() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
        -1.0f, -0.5f, 0.0f, // left  
         0.0f, -0.5f, 0.0f, // right 
        -0.5f,  0.5f, 0.0f,  // top
         1.0f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
        };

        unsigned int indices[] = {
            // ע��������0��ʼ! 
            // ����������(0,1,2,3)���Ƕ�������vertices���±꣬
            // �����������±��������ϳɾ���

            0, 1, 2, // ��һ��������
            1, 3, 4  // �ڶ���������
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        // glBindVertexArray(0);

    }

    void InitTwoTrangle() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
        -0.8f, -0.5f, 0.0f, // left  
        -0.2f, -0.5f, 0.0f, // right 
        -0.5f,  0.5f, 0.0f  // top
        };

        float vertices2[] = {
         0.2f, -0.5f, 0.0f, // right 
         0.8f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
        };

        unsigned int indices[] = {
            // ע��������0��ʼ! 
            // ����������(0,1,2,3)���Ƕ�������vertices���±꣬
            // �����������±��������ϳɾ���
            0, 1, 2
        };

        glGenVertexArrays(1, &VAO);
        glGenVertexArrays(1, &VAO2);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &VBO2);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(VAO2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

	void Draw() {
		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		// draw our first triangle
		glUseProgram(shaderProgram);
		// glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		glDrawArrays(GL_TRIANGLES, 0, 3);
        if (EBO) {
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

		// glBindVertexArray(0); // no need to unbind it every time 
	}

    void DrawTwo() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        if (EBO) {
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    void InitUniformDemo() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "out vec4 vertexColor; // ΪƬ����ɫ��ָ��һ����ɫ���\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "   vertexColor = vec4(0.5, 0.0, 0.0, 1.0); // �������������Ϊ����ɫ\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "uniform vec4 ourColor; // �Ӷ�����ɫ�����������������������ͬ��������ͬ��\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = ourColor;\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);

    }

    void DrawUniform() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // draw our first triangle
        glUseProgram(shaderProgram);

        float timeValue = glfwGetTime();
        float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        // glUseProgram(shaderProgram);
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void InitMultiProp() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor; // ��ɫ����������λ��ֵΪ 1\n"
            "out vec3 ourColor; // ��Ƭ����ɫ�����һ����ɫ\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "   ourColor = aColor; // ��ourColor����Ϊ���ǴӶ�����������õ���������ɫ\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 ourColor;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(ourColor, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
            // λ��              // ��ɫ
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // ����
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // ����
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // ����
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // ��ɫ����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0); 
    }


    void InitMultiPropMove() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor; // ��ɫ����������λ��ֵΪ 1\n"
            "out vec3 ourColor; // ��Ƭ����ɫ�����һ����ɫ\n"
            "uniform float yOffset;\n"
            "void main()\n"
            "{\n"
            "   vec3 pos = aPos;\n"
            "   if(gl_VertexID == 2) {\n"  // ��3�����㣨����Ϊ2���Ƕ�����
            "       pos.y *= yOffset;\n"
            "   }\n"
            "   gl_Position = vec4(pos, 1.0);\n"
            "   ourColor = aColor; // ��ourColor����Ϊ���ǴӶ�����������õ���������ɫ\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 ourColor;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(ourColor, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
            // λ��              // ��ɫ
             0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,   // ����
            -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,   // ����
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // ����
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // ��ɫ����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }

    void InitMultiPropLRMove() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor; // ��ɫ����������λ��ֵΪ 1\n"
            "out vec3 ourColor; // ��Ƭ����ɫ�����һ����ɫ\n"
            "uniform vec3 lrOffset;\n"
            "void main()\n"
            "{\n"
            "   vec3 pos = aPos + lrOffset;\n"
            "   gl_Position = vec4(pos, 1.0);\n"
            "   ourColor = aColor; // ��ourColor����Ϊ���ǴӶ�����������õ���������ɫ\n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 ourColor;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(ourColor, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
            // λ��              // ��ɫ
             0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,   // ����
            -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,   // ����
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // ����
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // ��ɫ����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }


    void DrawMultiMove() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // draw our first triangle
        glUseProgram(shaderProgram);
        int vertexOffset = glGetUniformLocation(shaderProgram, "yOffset");
        // glUseProgram(shaderProgram);
        glUniform1f(vertexOffset, cos(glfwGetTime() * 2.0f));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void DrawMultiLRMove() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // draw our first triangle
        glUseProgram(shaderProgram);
        int vertexOffset = glGetUniformLocation(shaderProgram, "lrOffset");
        // glUseProgram(shaderProgram);
        glUniform3f(vertexOffset, cos(glfwGetTime() * 2.0f), 0, 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void InitVecToClr() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor; // ��ɫ����������λ��ֵΪ 1\n"
            "out vec3 ourColor; // ��Ƭ����ɫ�����һ����ɫ\n"
            "uniform vec3 lrOffset;\n"
            "void main()\n"
            "{\n"
            "   vec3 pos = aPos + lrOffset;\n"
            "   gl_Position = vec4(pos, 1.0);\n"
            "   ourColor = abs(pos); \n"
            "}\0";
        vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 ourColor;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(ourColor, 1.0f);\n"
            "}\0";
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        float vertices[] = {
            // λ��              // ��ɫ
             0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,   // ����
            -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,   // ����
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // ����
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // ��ɫ����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }

    void DrawVecToClr() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // draw our first triangle
        glUseProgram(shaderProgram);
        int vertexOffset = glGetUniformLocation(shaderProgram, "lrOffset");
        // glUseProgram(shaderProgram);
        float time = glfwGetTime();
        float radiusX = 0.5f;  // x��뾶
        float radiusY = 0.3f;  // y��뾶
        glUniform3f(vertexOffset, cos(time) * radiusX, sin(time) * radiusY, 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void InitTexture() {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aColor;\n"
            "layout (location = 2) in vec2 aTexCoord;\n"
            "out vec3 ourColor;\n"
            "out vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos, 1.0);\n"
            "   ourColor = aColor;\n"
            "   TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
            "}\0";

        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec3 ourColor;\n"
            "in vec2 TexCoord;\n"
            "uniform sampler2D texture1;\n"
            "uniform sampler2D texture2;\n"
            "void main()\n"
            "{\n"
            "   FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2) * vec4(ourColor, 1.0);\n"
            "}\0";

        outshader = new Shader(vertexShaderSource, fragmentShaderSource, false);


        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        // ��Ҫ������container.jpgŶ����ַ�� https://learnopengl-cn.github.io/img/01/06/container.jpg 
        // ����һ��Ц�����ص�ַ��https://learnopengl-cn.github.io/img/01/06/awesomeface.png
        unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);

        float vertices[] = {
            //     ---- λ�� ----       ---- ��ɫ ----     - �������� -
                 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // ����
                 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // ����
                -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // ����
                -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // ����
        };

        unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texture coord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    void DrawTexture() {
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (outshader) {
            outshader->use();
        }
        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);

        // render container
        // ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

private:
	unsigned int vertexShader;
	unsigned int fragmentShader;
	unsigned int shaderProgram;
    unsigned int texture;

	unsigned int VAO;
	unsigned int VAO2;
    unsigned int VBO;
	unsigned int VBO2;
	unsigned int EBO;

    Shader* outshader;
};
