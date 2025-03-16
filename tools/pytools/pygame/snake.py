import pygame
import time
import random

# 初始化 Pygame
pygame.init()

# 定义颜色
white = (255, 255, 255)
yellow = (255, 255, 102)
black = (0, 0, 0)
red = (213, 50, 80)
green = (0, 255, 0)
blue = (50, 153, 213)

# 定义游戏窗口大小
dis_width = 800
dis_height = 600

# 创建游戏窗口
dis = pygame.display.set_mode((dis_width, dis_height))
pygame.display.set_caption('贪吃蛇游戏')

# 定义时钟对象
clock = pygame.time.Clock()

# 定义蛇的初始大小
snake_block = 10
# 定义不同难度等级的蛇速度
snake_speeds = [10, 15, 20, 25, 30]

# 定义字体样式
# 修改字体样式，使用支持中文的字体，确保系统中存在该字体，例如SimHei
# 若系统中没有SimHei字体，可以尝试使用其他中文字体，如WenQuanYi Zen Hei
font_style = pygame.font.SysFont("SimHei,WenQuanYi Zen Hei", 25)
score_font = pygame.font.SysFont("SimHei,WenQuanYi Zen Hei", 35)

def Your_score(score):
    value = score_font.render("你的分数: " + str(score), True, yellow)
    dis.blit(value, [0, 0])

def message(msg, color):
    mesg = font_style.render(msg, True, color)
    dis.blit(mesg, [dis_width / 6, dis_height / 3])

# 新增函数：选择难度等级
def select_difficulty():
    difficulty_selected = False
    while not difficulty_selected:
        dis.fill(blue)
        message("选择难度等级: 1-简单, 2-普通, 3-中等, 4-困难, 5-极难", yellow)
        pygame.display.update()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                quit()
            if event.type == pygame.KEYDOWN:
                if event.key in [pygame.K_1, pygame.K_2, pygame.K_3, pygame.K_4, pygame.K_5]:
                    difficulty = int(event.unicode) - 1
                    difficulty_selected = True
                    return snake_speeds[difficulty]

def our_snake(snake_block, snake_list):
    """
    绘制蛇的身体

    :param snake_block: 蛇身体每一块的大小
    :param snake_list: 蛇身体的坐标列表
    """
    for x in snake_list:
        pygame.draw.rect(dis, white, [x[0], x[1], snake_block, snake_block])

def gameLoop():
    # 获取用户选择的难度等级对应的速度
    snake_speed = select_difficulty()

    game_over = False
    game_close = False

    # 蛇的初始位置
    x1 = dis_width / 2
    y1 = dis_height / 2

    # 蛇每次移动的距离
    x1_change = 0
    y1_change = 0

    # 蛇的身体列表
    snake_List = []
    Length_of_snake = 1

    # 食物的随机位置
    foodx = round(random.randrange(0, dis_width - snake_block) / 10.0) * 10.0
    foody = round(random.randrange(0, dis_height - snake_block) / 10.0) * 10.0

    while not game_over:

        while game_close == True:
            dis.fill(blue)
            message("你输了！按 Q-退出 或 C-重新开始", red)
            Your_score(Length_of_snake - 1)
            pygame.display.update()

            for event in pygame.event.get():
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_q:
                        game_over = True
                        game_close = False
                    if event.key == pygame.K_c:
                        gameLoop()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                game_over = True
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_LEFT:
                    x1_change = -snake_block
                    y1_change = 0
                elif event.key == pygame.K_RIGHT:
                    x1_change = snake_block
                    y1_change = 0
                elif event.key == pygame.K_UP:
                    y1_change = -snake_block
                    x1_change = 0
                elif event.key == pygame.K_DOWN:
                    y1_change = snake_block
                    x1_change = 0

        # 检查蛇是否撞到边界
        if x1 >= dis_width or x1 < 0 or y1 >= dis_height or y1 < 0:
            game_close = True
        x1 += x1_change
        y1 += y1_change
        dis.fill(blue)
        pygame.draw.rect(dis, green, [foodx, foody, snake_block, snake_block])
        snake_Head = []
        snake_Head.append(x1)
        snake_Head.append(y1)
        snake_List.append(snake_Head)
        if len(snake_List) > Length_of_snake:
            del snake_List[0]

        # 检查蛇是否撞到自己
        for x in snake_List[:-1]:
            if x == snake_Head:
                game_close = True

        our_snake(snake_block, snake_List)
        Your_score(Length_of_snake - 1)

        pygame.display.update()

        # 检查蛇是否吃到食物
        if x1 == foodx and y1 == foody:
            foodx = round(random.randrange(0, dis_width - snake_block) / 10.0) * 10.0
            foody = round(random.randrange(0, dis_height - snake_block) / 10.0) * 10.0
            Length_of_snake += 1

        clock.tick(snake_speed)

    pygame.quit()
    quit()


gameLoop()
