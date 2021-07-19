import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { faInfoCircle} from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';


@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'ZX Interface Z';
  faInfoCircle = faInfoCircle;
  faGithub = faGithub;

  constructor(
    private route: ActivatedRoute
  ) { }

  ngOnInit(): void {
    // console.log(his.route.snapshot.
  }
}
