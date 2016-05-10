/*
 Copyright (c) 2013 OpenSourceRF.com.  All right reserved.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#import <UIKit/UIKit.h>

#import "RFduino.h"

@interface AppViewController : UIViewController<RFduinoDelegate, UITextFieldDelegate>
{
    __weak IBOutlet UIImageView *colorWheel;
    
    __weak IBOutlet UIButton *colorSwatch;
    
    __weak IBOutlet UILabel *rLabel;
    __weak IBOutlet UILabel *gLabel;
    __weak IBOutlet UILabel *bLabel;
    
    __weak IBOutlet UISlider *rSlider;
    __weak IBOutlet UISlider *gSlider;
    __weak IBOutlet UISlider *bSlider;
    
    __weak IBOutlet UITextField *rValue;
    __weak IBOutlet UITextField *gValue;
    __weak IBOutlet UITextField *bValue;
    
    float red;
    float green;
    float blue;
}

@property(strong, nonatomic) RFduino *rfduino;

- (void) pickedColor:(UIColor*)color;

- (IBAction)rSliderChanged:(id)sender;
- (IBAction)gSliderChanged:(id)sender;
- (IBAction)bSliderChanged:(id)sender;

- (IBAction)rEditingDidEnd:(id)sender;
- (IBAction)gEditingDidEnd:(id)sender;
- (IBAction)bEditingDidEnd:(id)sender;

@end
