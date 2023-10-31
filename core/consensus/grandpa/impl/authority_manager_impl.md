
## Grandpa voter set
Voting signs voter set id.  
Digests which change voter set do not include voter set id.  
Implementations must keep track of all changes to correctly verify signatures.  
There are two types of digests.  
Scheduled change increments id of voter set at block before that digest.  
Forced change increments id of voter set at specified block.  
Forced change ignores scheduled changes after specified block and before that digest.  

Justification for block with scheduled change ignored by forced change is ambigous.  
Westend 12927502 has forced change to block 12927152 set 5671.  
But there are justifications for blocks 12927234 set 5672 and block 12927278 set 5673.  
So we assume that if such blocks are signed by previous voter set then forced change specified block is modified.  

## Westend forced changes
<table>
  <tr>
    <th>before</th>
    <th>forced</th>
    <th>after</th>
  </tr>
  <tr>
    <td>1416901<br>S 2447 → 2448</td>
    <td>1491033<br>F 2448 → 2449</td>
    <td>1492211<br>S 2449 → 2450</td>
  </tr>
  <tr>
    <td>2300366<br>S 2683 → 2684</td>
    <td>2318338<br>F 2684 → 2685</td>
    <td>2321061<br>S 2685 → 2686</td>
  </tr>
  <tr>
    <td>2437426<br>S 2721 → 2722</td>
    <td>2437992<br>F 2722 → 2723</td>
    <td>2440808<br>S 2723 → 2724</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">4356314<br>S 3269 → 3270</td>
    <td>4356716<br>F 3270 → 3271</td>
    <td rowspan="2">4360992<br>S 3272 → 3273</td>
  </tr>
  <tr>
    <td>4357394<br>F 3271 → 3272</td>
  </tr>
  <tr>
    <td>12688814<br>S 5601 → 5602</td>
    <td>12691801<br>F 5602 → 5603</td>
    <td>12691821<br>S 5603 → 5604</td>
  </tr>
  <tr>
    <td>12718597<br>S 5611 → 5612</td>
    <td>12719004<br>F 5612 → 5613</td>
    <td>12734442<br>S 5613 → 5614</td>
  </tr>
  <tr valign="top">
    <td rowspan="4">12734442<br>S 5613 → 5614</td>
    <td>12734588<br>F 5614 → 5615</td>
    <td rowspan="4">12739027<br>S 5618 → 5619</td>
  </tr>
  <tr>
    <td>12735037<br>F 5615 → 5616</td>
  </tr>
  <tr>
    <td>12735379<br>F 5616 → 5617</td>
  </tr>
  <tr>
    <td>12735523<br>F 5617 → 5618</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">12927278<br>S 5673 → 5674</td>
    <td>12927502<br>F 5674 → 5675</td>
    <td rowspan="2">12929972<br>S 5676 → 5677</td>
  </tr>
  <tr>
    <td>12927596<br>F 5675 → 5676</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">15532335<br>S 6411 → 6412</td>
    <td>15533486<br>F 6412 → 6413</td>
    <td rowspan="2">15693226<br>S 6414 → 6415</td>
  </tr>
  <tr>
    <td>15533936<br>F 6413 → 6414</td>
  </tr>
</table>
