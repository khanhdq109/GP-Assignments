import pygame
import random
import sys
import time

class Hole():
    def __init__(self, position = (500, 500)):
        self.zom = False
        self.position = position
        self.appear_time = 0
    
    # A new zombie appears or smashes a zombie
    def trigger(self):
        self.zom = not self.zom
        if self.zom:
            self.appear_time = time.time()
        
    # True if hole already had a zombie, otherwise false
    def has_zombie(self):
        return self.zom
    
    # Disappear after a duration
    def check_disappear(self, current_time, exist_time):
        return self.zom and (current_time - self.appear_time >= exist_time)

class Game():
    def __init__(self):
        # Score & level
        self.score = 0
        self.level = 1
        self.add = [2, 4, 6, 8, 10, 12, 14, 16, 18, 20]
        self.life = 5
        self.thres = [20, 60, 120, 200, 300]
        
        # Holes
        self.positions = [
            (200, 160), (500, 160), (100, 280), (600, 280), (350, 100),
            (350, 500)
        ]
        self.max_hole = 3
        self.holes = [Hole((350, 280)), Hole((200, 400)), Hole((500, 400))]
        self.exist_time = 2.5
        
        # Screen size
        self.screen_width = 800
        self.screen_height = 600
        
        # Background music
        self.background_music = "music/background.mp3"
        self.smash_music = "music/smash.mp3"
        
        # Images
        self.background_image = pygame.image.load("img/background.png")
        self.background_image = pygame.transform.scale(self.background_image, (self.screen_width, self.screen_height))
        self.hole_image = pygame.image.load("img/hole.png")
        self.hole_image = pygame.transform.scale(self.hole_image, (100, 100))
        self.zombie_image = pygame.image.load("img/zom.png")
        self.zombie_image = pygame.transform.scale(self.zombie_image, (100, 100))
        self.smash_image = pygame.image.load("img/smash.png")  
        self.smash_image = pygame.transform.scale(self.smash_image, (100, 100))
        self.heart_image = pygame.image.load("img/heart.png")  
        self.heart_image = pygame.transform.scale(self.heart_image, (30, 30))
        
        # Smash effect variables
        self.smash_effect = False
        self.smash_position = (0, 0)
        self.smash_timer = 0
        
        # Fonts for score and level
        pygame.font.init()
        self.font = pygame.font.SysFont(None, 36)  # Default font with size 36
        self.game_over_font = pygame.font.SysFont(None, 72)  # Larger font for "GAME OVER"
        
        # Others
        self.last_hit = (0, 0)
        self.clock = pygame.time.Clock()
        self.reset_interval = 5
        self.FPS = 30
    
    # Reset zombies in empty holes
    def reset_hole(self):
        tmp = []
        for i in range(2):
            idx = random.randint(0, self.max_hole - 1)
            if self.holes[idx].position == self.last_hit or self.holes[idx].has_zombie():  # Check if hole already has zombie
                continue
            tmp.append(idx)
                
        active_zombies = sum([1 for hole in self.holes if hole.has_zombie()])
        for idx in tmp:
            if (not self.holes[idx].has_zombie()) and (active_zombies < self.max_hole):
                if random.choice([0, 1]) == 1:
                    self.holes[idx].trigger()
                    active_zombies += 1

    def is_zombie_hit(self, mouse_x, mouse_y):
        for hole in self.holes:
            if hole.has_zombie():
                zom_x, zom_y = hole.position[0], hole.position[1] - 50
                if zom_x <= mouse_x <= zom_x + 100 and zom_y <= mouse_y <= zom_y + 100:
                    hole.trigger()
                    self.last_hit = hole.position
                    return True, hole.position
        return False, (0, 0)

    def check_level(self):
        if self.score >= self.thres[self.level - 1]:
            return True
        return False
        
    def level_up(self):
        if self.level < len(self.thres): 
            self.max_hole += 1
            self.holes.append(Hole(self.positions[self.level - 1]))
            self.exist_time -= 0.2
            self.level += 1 

    # Render text on screen
    def render_text(self, text, position, font):
        text_surface = font.render(text, True, (255, 11, 11))  # Red color
        self.screen.blit(text_surface, position)

    # Show game over screen
    def show_game_over(self):
        self.screen.blit(self.background_image, (0, 0))
        
        overlay = pygame.Surface((self.screen_width, self.screen_height))
        overlay.set_alpha(128)  
        overlay.fill((200, 0, 0))
        self.screen.blit(overlay, (0, 0))
        
        # Render "GAME OVER" text
        self.render_text("GAME OVER", (self.screen_width // 2 - 150, self.screen_height // 2 - 50), self.game_over_font)
        self.render_text("SCORE: " + str(self.score), (self.screen_width // 2 - 150, self.screen_height // 2 + 50), self.game_over_font)
        
        pygame.display.update()
        time.sleep(6)
        pygame.quit()
        sys.exit()
        
    def draw_lives(self):
            heart_width, heart_height = self.heart_image.get_size()
            margin = 10  # Margin between hearts
            x = self.screen_width - heart_width - margin
            y = margin
            
            for _ in range(self.life):
                self.screen.blit(self.heart_image, (x, y))
                x -= heart_width + margin
    
    def run(self):
            # Initialize the game
            pygame.init()
            pygame.mixer.init()
            self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
            pygame.display.set_caption("Zombie Smasher")
            pygame.mixer.music.load(self.background_music)
            pygame.mixer.music.play(-1)
            self.smash_music = pygame.mixer.Sound(self.smash_music)
            
            # Loop
            running = True
            while running:
                if self.life <= 0:
                    self.show_game_over()
                
                # Reset hole
                if pygame.time.get_ticks() % self.reset_interval == 0:
                    self.reset_hole()
                    
                # Check if zombies disappear
                current_time = time.time()
                for hole in self.holes:
                    if hole.check_disappear(current_time, self.exist_time):
                        self.life -= 1
                        hole.trigger()
                
                # Draw background image
                self.screen.blit(self.background_image, (0, 0))
                
                # Draw holes
                for hole in self.holes:
                    self.screen.blit(self.hole_image, hole.position)
                
                # Draw zombies
                for hole in self.holes:
                    if hole.has_zombie():
                        self.screen.blit(self.zombie_image, (hole.position[0], hole.position[1] - 50))
                
                # Draw smash effect if active
                if self.smash_effect:
                    self.screen.blit(self.smash_image, self.smash_position)
                    self.smash_timer -= 1
                    if self.smash_timer <= 0:
                        self.smash_effect = False
                
                # Display score, level, and life
                self.render_text(f"Level: {self.level}", (10, 10), self.font)
                self.render_text(f"Score: {self.score}", (10, 50), self.font)
                
                # Draw the heart symbols representing lives
                self.draw_lives()
                
                # Handle events
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        running = False
                        pygame.quit()
                        sys.exit()
                        
                    if event.type == pygame.MOUSEBUTTONDOWN:
                        mouse_x, mouse_y = event.pos
                        hitted, pos = self.is_zombie_hit(mouse_x, mouse_y)
                        if hitted:
                            self.score += self.add[self.level - 1]
                            self.smash_music.play()
                            
                            # Trigger the smash effect
                            self.smash_effect = True
                            self.smash_position = (pos[0], pos[1] - 50)
                            self.smash_timer = 10  # Display the smash effect for 10 frames
                
                # Check the level
                if self.check_level():
                    self.level_up()
                  
                # Update the display
                pygame.display.update()
                
                # Control the frame rate
                self.clock.tick(self.FPS)
                
if __name__ == "__main__":
    game = Game()
    game.run()